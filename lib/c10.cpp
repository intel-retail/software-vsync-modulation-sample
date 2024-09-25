/*
 * Copyright © 2024 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include <math.h>
#include <debug.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "mmio.h"
#include "c10.h"

c10_phy_reg c10_table[] = {
	{REG(PORT_CLOCK_CTL(0)), REG(TRANS_DDI_FUNC_CTL_A), REG(PORT_M2P_MSGBUS_CTL(0)), REG(_DDI_CLK_VALFREQ_A), INTEL_OUTPUT_UNUSED, 0, 1},
	{REG(PORT_CLOCK_CTL(1)), REG(TRANS_DDI_FUNC_CTL_B), REG(PORT_M2P_MSGBUS_CTL(1)), REG(_DDI_CLK_VALFREQ_B), INTEL_OUTPUT_UNUSED, 0, 1},
};

/**
 * @brief
 * Constructor for c10 class
 * @param ds
 */
c10::c10(ddi_sel *ds)
{
	u32 port = ds->de_clk - 1;

	int pll_state;
	for(int i = 0; i < 20; i++) {
		pll_state = __intel_cx0_read(port, 0, PHY_C10_VDR_PLL(i));
		DBG("c10: pll_state[%d] = 0x%X, %d\n",i, pll_state,pll_state);
	}

	ds->phy_data = &c10_table[0];
	set_ds(ds);
	set_init(true);

}

/**
 * @brief
 * This is an inline function to multiply two 32-bit numbers
 * and return the result as a 64-bit number.
 * @param a - The first 32-bit number
 * @param b - The second 32-bit number
 */
static inline unsigned long long mul_u32_u32(u32 a, u32 b)
{
	return (unsigned long long)a * b;
}


/**
* @brief
* This function calculates pll values to achieve desired drift using the given shift value.
* It programs the MMIO registers of the PHY to achieve the desired drift and also sets a timer
* to reset the MMIO registers to their original values after a certain time period.
* @param time_diff - This is the time difference in between the primary and the
* secondary systems in ms. If master is ahead of the slave , then the time
* difference is a positive number otherwise negative.
* @param shift - Fraction value to be used during the calculation.
* @return void
*/
void c10::program_phy(double time_diff, double shift)
{
	ddi_sel *ds = get_ds();
	c10_phy_reg *c10_phy = (c10_phy_reg *) ds->phy_data;

	int pll_state[20];
	int mpll_frac_rem_15_0, mpll_frac_den_15_0, mpll_frac_quot_15_0, mpll_multiplier_11_0, ref_clk_mpll_div_2_0;
	int pll_freq;

	for(int i = 0; i < 20; i++) {
		pll_state[i] = __intel_cx0_read(ds->de_clk - 1, 0, PHY_C10_VDR_PLL(i));
		c10_phy->c10.pll_old[i]  = pll_state[i];
		DBG("c10: pll_state[%d] = 0x%X, %d\n", i, pll_state[i],pll_state[i]);
	}

	c10_phy->done =0;

	if(time_diff < 0) {
		shift *= -1;
	}

	int steps = CALC_STEPS_TO_SYNC(time_diff, shift);
	DBG("steps are %d\n", steps);
	user_info *ui = new user_info(this, c10_phy);
	make_timer((long) steps, ui, reset_phy_regs);

	mpll_frac_den_15_0 = pll_state[10] << 8 | pll_state[9];
	mpll_frac_quot_15_0 = pll_state[12] << 8 | pll_state[11];
	mpll_frac_rem_15_0 = pll_state[14] << 8 | pll_state[13];
	mpll_multiplier_11_0 = (REG_FIELD_GET8(C10_PLL3_MULTIPLIERH_MASK, pll_state[3]) << 8 |
			pll_state[2]) / 2 + 16;
	ref_clk_mpll_div_2_0 = REG_FIELD_GET8(C10_PLL15_TXCLKDIV_MASK, pll_state[15]);

	pll_freq = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(REF_CLK_FREQ * 1000, (mpll_multiplier_11_0 << 16) + mpll_frac_quot_15_0) +
			DIV_ROUND_CLOSEST(REF_CLK_FREQ * 1000 * mpll_frac_rem_15_0, mpll_frac_den_15_0),
			10 << (ref_clk_mpll_div_2_0 + 16));

	double new_pll_freq = pll_freq + (shift * pll_freq / 100);
	unsigned long long temp = new_pll_freq * (10 << (ref_clk_mpll_div_2_0 + 16));
	temp -= DIV_ROUND_CLOSEST(REF_CLK_FREQ * 1000 * mpll_frac_rem_15_0, mpll_frac_den_15_0);
	temp /= (REF_CLK_FREQ * 1000);
	temp -= (mpll_multiplier_11_0 << 16);
	int new_mpll_frac_quot_15_0 = (int) temp;
	pll_state[11] = new_mpll_frac_quot_15_0 & GENMASK(7, 0);
	pll_state[12] =  new_mpll_frac_quot_15_0 >> 8;

	DBG("Old Quotient = 0x%X\n", mpll_frac_quot_15_0);
	DBG("Denominator = 0x%X\n", mpll_frac_den_15_0);
	DBG("Remainder = 0x%X\n", mpll_frac_rem_15_0);
	DBG("Multiplier = 0x%X\n", mpll_multiplier_11_0);

	c10_phy->c10.pll[11] = pll_state[11];
	c10_phy->c10.pll[12] = pll_state[12];
	program_mmio(c10_phy, 1);
}

/**
* @brief
* This is a helper class function to act as callback and triggers resetting of C10
* Phy MMIO registers to their original value. It gets executed whenever a timer expires.
* We program MMIO registers of the PHY in this function becase we have waited for a certain
* time period to get the primary and secondary systems vsync in sync and now
* it is time to reprogram the default values for the secondary system's PHYs.
* @param sig - The signal that fired
* @param *si - A pointer to a siginfo_t, which is a structure containing
* further information about the signal
* @param *uc - This is a pointer to a ucontext_t structure, cast to void *.
* The structure pointed to by this field contains signal context information
* that was saved on the user-space stack by the kernel
* @return void
*/
void c10::reset_phy_regs(int sig, siginfo_t *si, void *uc)
{
	user_info *ui = (user_info *) si->si_value.sival_ptr;
	if(!ui) {
		return;
	}

	DBG("timer done\n");
	c10_phy_reg *dr = (c10_phy_reg *) ui->get_reg();
	c10 *d = (c10 *) ui->get_type();

	d->reset_phy_regs(dr);
	delete ui;
}

/**
* @brief
* This is an object function called by the timer callback and actually
* calls program_mmio().
* @param dr - c10 phy register object
* @return void
*/
 void c10::reset_phy_regs(c10_phy_reg *dr)
{
	program_mmio(dr, 0);
	dr->done = 1;
}


/**
* @brief
* This function waits until the C10 programming is
* finished. There is a timer for which time the new values will remain in
* effect. After that timer expires, the original values will be restored.
* @param t - The timer which needs to be deleted
* @return void
*/
void c10::wait_until_done()
{
	TRACING();

	ddi_sel *ds = get_ds();
	c10_phy_reg *c10_phy = (c10_phy_reg *) ds->phy_data;

	while(!c10_phy->done && !lib_client_done) {
		usleep(1000); // Wait for 1 ms
	}

	// Restore original values in case of app termination
	if (lib_client_done) {
		reset_phy_regs(c10_phy);
	}

	timer_delete(get_timer());
}

/**
* @brief
* This function programs the MMIO (Memory-Mapped I/O) registers of the C10 PHY.
* It writes the specified values to the PHY's MMIO registers to configure its operation.
*
* @param pr - A pointer to a c10_phy_reg structure, which holds the MMIO register values to be programmed.
* @param mod - A modifier value that affects how the MMIO registers are programmed.
* @return void
*/
void c10::program_mmio(c10_phy_reg *pr, int mod)
{

	ddi_sel *ds = get_ds();
	u32 port = ds->de_clk - 1 ;

	intel_cx0_rmw(port, INTEL_CX0_BOTH_LANES, PHY_C10_VDR_CONTROL(1),
		0, C10_VDR_CTRL_MSGBUS_ACCESS,
		MB_WRITE_COMMITTED);

	/* Custom width needs to be programmed to 0 for both the phy lanes */
	intel_cx0_rmw(port, INTEL_CX0_BOTH_LANES, PHY_C10_VDR_CUSTOM_WIDTH,
			C10_VDR_CUSTOM_WIDTH_MASK, C10_VDR_CUSTOM_WIDTH_8_10,
			MB_WRITE_COMMITTED);
	intel_cx0_rmw(port, INTEL_CX0_BOTH_LANES, PHY_C10_VDR_CONTROL(1),
			0, C10_VDR_CTRL_UPDATE_CFG,
			MB_WRITE_COMMITTED);


	__intel_cx0_write(port, 0, PHY_C10_VDR_PLL(11), mod ? pr->c10.pll[11] : pr->c10.pll_old[11] , MB_WRITE_COMMITTED);
	__intel_cx0_write(port, 0, PHY_C10_VDR_PLL(12), mod ? pr->c10.pll[12] : pr->c10.pll_old[12], MB_WRITE_COMMITTED);

	intel_cx0_rmw(port, INTEL_CX0_LANE0, PHY_C10_VDR_CONTROL(1),
			0, C10_VDR_CTRL_MASTER_LANE | C10_VDR_CTRL_UPDATE_CFG,
			MB_WRITE_COMMITTED);

}


/**
 * @brief
 * This routine waits until the target register @reg contains the expected
 * @value after applying the @mask, i.e. it waits until ::
 *
 *     (intel_uncore_read_fw(uncore, reg) & mask) == value
 *
 * Otherwise, the wait will timeout after @slow_timeout_ms milliseconds.
 * For atomic context @slow_timeout_ms must be zero and @fast_timeout_us
 * must be not larger than 20,0000 microseconds.
 *
 * Note that this routine assumes the caller holds forcewake asserted, it is
 * not suitable for very long waits. See intel_wait_for_register() if you
 * wish to wait without holding forcewake for the duration (i.e. you expect
 * the wait to be slow).
 *
 * @param uncore - the struct intel_uncore
 * @param reg - the register to read
 * @param mask - mask to apply to register value
 * @param value - expected value
 * @param fast_timeout_us - fast timeout in microsecond for atomic/tight wait
 * @param slow_timeout_ms - slow timeout in millisecond
 * @param out_value - optional placeholder to hold registry value
 * @return int - 0 if the register matches the desired condition, or -ETIMEDOUT.
 *
 */
int c10::__intel_wait_for_register_fw(
				 u32 reg,
				 u32 mask,
				 u32 value,
				 u32 *out_value)
{
	u32 reg_value = 0;
	int ret = 0;

	reg_value = READ_OFFSET_DWORD(reg);

	if (out_value)
		*out_value = reg_value;

	return ret;
}

/**
 * @brief
 * This routine waits until the target register @reg contains the expected
 * @value after applying the @mask, i.e. it waits until ::
 *
 *     (intel_uncore_read(uncore, reg) & mask) == value
 *
 * Otherwise, the wait will timeout after @timeout_ms milliseconds.
 *
 * @param reg - the register to read
 * @param mask - mask to apply to register value
 * @param value - expected value
 * @param fast_timeout_us - fast timeout in microsecond for atomic/tight wait
 * @param slow_timeout_ms - slow timeout in millisecond
 * @param out_value - optional placeholder to hold registry value
 * @return int - 0 if the register matches the desired condition, or -ETIMEDOUT.
 */
int c10::__intel_wait_for_register(
				  u32 reg,
				  u32 mask,
				  u32 value,
				  u32 *out_value)
{
	u32 reg_value;
	int ret;

	ret = __intel_wait_for_register_fw(reg, mask, value, &reg_value);

	if (out_value)
		*out_value = reg_value;

	return ret;
}

/**
 * @brief
 *  Wait for register to match expected value
 * @param reg - register to wait on
 * @param mask - mask to apply to register value
 * @param value - expected value
 * @param timeout_ms -
 * @return int - 0 if the register matches the desired condition, or -ETIMEDOUT.
 */
int c10::intel_wait_for_register(u32 reg,
			u32 mask,
			u32 value,
			unsigned int timeout_ms)
{
	return __intel_wait_for_register(reg, mask, value, NULL);
}

/**
 * @brief
 * This function waits until the target register @reg contains the expected
 * @param reg - register to wait on
 * @param mask - mask to apply to register value
 * @param value - expected value
 * @param out_value - optional placeholder to hold registry value
 * @return int - 0 if the register matches the desired condition, or -ETIMEDOUT.
 */
int c10::__intel_de_wait_for_register(u32 reg,
				 u32 mask, u32 value,
				 u32 *out_value)
{
	int out = __intel_wait_for_register(reg, mask, value, out_value);
	if(out_value) {
		DBG("Wait: reg: 0x%X, mask = 0x%X, value = 0x%X, out_value = 0x%X\n", reg, mask, value, *out_value);
	} else {
		DBG("Wait: reg: 0x%X, mask = 0x%X, value = 0x%X\n", reg, mask, value);
	}
	return out;
}

/**
 * @brief
 * This function waits until the target register @reg contains the expected
 * @param reg - register to wait on
 * @param mask - mask to apply to register value
 * @param value - expected value
 * @param timeout - timeout in milliseconds
 * @return int - 0 if the register matches the desired condition, or -ETIMEDOUT.
 */
int c10::intel_de_wait_for_register(u32 reg, u32 mask, u32 value, unsigned int timeout)
{
	return intel_wait_for_register(reg, mask, value, timeout);
}

/**
 * @brief
 * This function performs read modify write operation. It clears certain bits
 * from the register.
 * @param reg - The address or identifier of the register to be modified.
 * @param clear - A bitmask that specifies which bits to clear (set to 0) in the register.
 * @param set - A bitmask that specifies which bits to set (set to 1) in the register.
 * @return u32 - old value
 */
u32 c10::intel_uncore_rmw(u32 reg, u32 clear, u32 set)
{
	u32 old, val;

	old = READ_OFFSET_DWORD(reg);
	val = (old & ~clear) | set;
	WRITE_OFFSET_DWORD(reg, val);
	return old;
}

/**
 * @brief
 * This function performs read modify write operation by calling intel_uncore_rmw.
 * @param reg - The address or identifier of the register to be modified.
 * @param clear - A bitmask that specifies which bits to clear (set to 0) in the register.
 * @param set - A bitmask that specifies which bits to set (set to 1) in the register.
 * @return u32 - old value
 */
u32 c10::intel_de_rmw(u32 reg, u32 clear, u32 set)
{
	DBG("RMW: reg: 0x%X, clear = 0x%X, set = 0x%X\n", reg, clear, set);
	return intel_uncore_rmw(reg, clear, set);
}

/**
 * @brief
 * This function waits until the target register @reg is cleared.
 * @param reg - register to wait on
 * @param mask - mask to apply to register value
 * @param timeout - timeout in milliseconds
 * @return int - 0 if the register matches the desired condition, or -ETIMEDOUT.
 */
int c10::intel_de_wait_for_clear(int reg, u32 mask, unsigned int timeout)
{
	return intel_de_wait_for_register(reg, mask, 0, timeout);
}

/**
 * @brief
 * This function reads the value from the register.
 * @param reg - The address or identifier of the register to be read.
 * @return u32 -
 */
u32 c10::intel_de_read(u32 reg)
{
	u32 val = READ_OFFSET_DWORD(reg);
	DBG("%s: %d reg: 0x%X, value = 0x%X\n", __FUNCTION__, __LINE__, reg, val);
	return val;
}

/**
 * @brief
 * This function writes the value to the register.
 * @param reg - The address or identifier of the register to be written.
 * @param val - The value to be written to the register.
 */
void c10::intel_de_write(u32 reg, u32 val)
{
	DBG("Write: reg: 0x%X, val = 0x%X\n", reg, val);
	WRITE_OFFSET_DWORD(reg, val);
}

/**
 * @brief
 * This function clears response ready flag by calling other helper function.
 * @param port - port number
 * @param lane - lane number
 */
void c10::intel_clear_response_ready_flag(u32 port, int lane)
{
	intel_de_rmw(XELPDP_PORT_P2M_MSGBUS_STATUS(port, lane),
			 0, XELPDP_PORT_P2M_RESPONSE_READY | XELPDP_PORT_P2M_ERROR_SET);
}

/**
 * @brief
 * This function resets the bus by writing desired values to registers.
 * @param port - port
 * @param lane - lane
 */
void c10::intel_cx0_bus_reset(int port, int lane)
{
	intel_de_write(XELPDP_PORT_M2P_MSGBUS_CTL(port, lane),
			   XELPDP_PORT_M2P_TRANSACTION_RESET);

	if (intel_de_wait_for_clear(XELPDP_PORT_M2P_MSGBUS_CTL(port, lane),
					XELPDP_PORT_M2P_TRANSACTION_RESET,
					XELPDP_MSGBUS_TIMEOUT_SLOW)) {
		ERR("Failed to bring port %d to idle.\n", port);
		return;
	}

	intel_clear_response_ready_flag(port, lane);
}

/**
 * @brief
 * This function waits for the acknowledgement from the device.
 * @param port - port
 * @param command - command to send
 * @param lane - lane
 * @param val - value
 * @return int
 */
int c10::intel_cx0_wait_for_ack(u32 port, int command, int lane, u32 *val)
{
	if (__intel_de_wait_for_register(
					 XELPDP_PORT_P2M_MSGBUS_STATUS(port, lane),
					 XELPDP_PORT_P2M_RESPONSE_READY,
					 XELPDP_PORT_P2M_RESPONSE_READY,
					 val)) {
		ERR("Port %d Timeout waiting for message ACK. Status: 0x%x\n", port, *val);
		return -ETIMEDOUT;
	}

	if (*val & XELPDP_PORT_P2M_ERROR_SET) {
		ERR("Port %d Error occurred during %s command. Status: 0x%x\n", port,
				command == XELPDP_PORT_P2M_COMMAND_READ_ACK ? "read" : "write", *val);
		intel_cx0_bus_reset(port, lane);
		return -EINVAL;
	}

	if (REG_FIELD_GET(XELPDP_PORT_P2M_COMMAND_TYPE_MASK, *val) != (u32) command) {
		ERR("Port %d Not a %s response. MSGBUS Status: 0x%x.\n", port,
				command == XELPDP_PORT_P2M_COMMAND_READ_ACK ? "read" : "write", *val);
		intel_cx0_bus_reset(port, lane);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief
 * This function reads the value from c10 registers using bus mechanism introduced in MTL/PTL.
 * @param port - target port
 * @param lane - target lane
 * @param addr - address
 * @return int
 */
int c10::__intel_cx0_read_once(u32 port, int lane, u16 addr)
{
	int ack;
	u32 val;

	if (intel_de_wait_for_clear(XELPDP_PORT_M2P_MSGBUS_CTL(port, lane),
					XELPDP_PORT_M2P_TRANSACTION_PENDING,
					XELPDP_MSGBUS_TIMEOUT_SLOW)) {
		ERR("Port %d Timeout waiting for previous transaction to complete. Reset the bus and retry.\n", port);
		intel_cx0_bus_reset(port, lane);
		return -ETIMEDOUT;
	}

	intel_de_write(XELPDP_PORT_M2P_MSGBUS_CTL(port, lane),
			   XELPDP_PORT_M2P_TRANSACTION_PENDING |
			   XELPDP_PORT_M2P_COMMAND_READ |
			   XELPDP_PORT_M2P_ADDRESS(addr));

	ack = intel_cx0_wait_for_ack(port, XELPDP_PORT_P2M_COMMAND_READ_ACK, lane, &val);
	if (ack < 0) {
		intel_cx0_bus_reset(port, lane);
		return ack;
	}

	intel_clear_response_ready_flag(port, lane);

	return REG_FIELD_GET(XELPDP_PORT_P2M_DATA_MASK, val);
}

/**
 * @brief
 * This function reads the value from c10 registers. It's a wrapper call to __intel_cx0_read_once
 * multiple times for accurate reading.
 * @param port - target port
 * @param lane - lane
 * @param addr - address
 * @return u8 - register value
 */
u8 c10::__intel_cx0_read(u32 port, int lane, u16 addr)
{
	int i, status;

	/* 3 tries is assumed to be enough to read successfully */
	for (i = 0; i < 3; i++) {
		status = __intel_cx0_read_once(port, lane, addr);

		if (status >= 0)
			return status;
	}

	ERR("Port %d Read %04x failed after %d retries.\n",
			 port, addr, i);

	return 0;
}

/**
 * @brief
 * This function writes the value to c10 registers using bus mechanism introduced in MTL/PTL.
 * @param port - target port
 * @param lane - lane
 * @param addr - address
 * @param data - value to write
 * @param committed - flag to indicate if the write is committed or not
 * @return int - 0 on success, negative value on failure
 */
int c10::__intel_cx0_write_once(u32 port, int lane, u16 addr, u8 data, bool committed)
{
	u32 val;

	if (intel_de_wait_for_clear(XELPDP_PORT_M2P_MSGBUS_CTL(port, lane),
					XELPDP_PORT_M2P_TRANSACTION_PENDING,
					XELPDP_MSGBUS_TIMEOUT_SLOW)) {
		ERR("Port %d Timeout waiting for previous transaction to complete. Resetting the bus.\n", port);
		intel_cx0_bus_reset(port, lane);
		return -ETIMEDOUT;
	}

	intel_de_write(XELPDP_PORT_M2P_MSGBUS_CTL(port, lane),
			   XELPDP_PORT_M2P_TRANSACTION_PENDING |
			   (committed ? XELPDP_PORT_M2P_COMMAND_WRITE_COMMITTED :
					XELPDP_PORT_M2P_COMMAND_WRITE_UNCOMMITTED) |
			   XELPDP_PORT_M2P_DATA(data) |
			   XELPDP_PORT_M2P_ADDRESS(addr));

	if (intel_de_wait_for_clear(XELPDP_PORT_M2P_MSGBUS_CTL(port, lane),
					XELPDP_PORT_M2P_TRANSACTION_PENDING,
					XELPDP_MSGBUS_TIMEOUT_SLOW)) {
		ERR("Port %d Timeout waiting for write to complete. Resetting the bus.\n", port);
		intel_cx0_bus_reset(port, lane);
		return -ETIMEDOUT;
	}

	if (committed) {
		if (intel_cx0_wait_for_ack(port, XELPDP_PORT_P2M_COMMAND_WRITE_ACK, lane, &val) < 0) {
			intel_cx0_bus_reset(port, lane);
			return -EINVAL;
		}
	} else if ((intel_de_read(XELPDP_PORT_P2M_MSGBUS_STATUS(port, lane)) &
			XELPDP_PORT_P2M_ERROR_SET)) {
		ERR("Port %d Error occurred during write command.\n", port);
		intel_cx0_bus_reset(port, lane);
		return -EINVAL;
	}

	intel_clear_response_ready_flag(port, lane);

	return 0;
}

/**
 * @brief
 * This function writes value to c10 registers. It's a wrapper call to __intel_cx0_write_once
 * multiple times for accurate writing.
 * @param port - target port
 * @param lane - lane
 * @param addr - address
 * @param data - value to write
 * @param committed - flag to indicate if the write is committed or not
 * @return u8 - register value
 */
void c10::__intel_cx0_write(u32 port, int lane, u16 addr, u8 data, bool committed)
{
	int i, status;

	 /* 3 tries is assumed to be enough to write successfully */
	for (i = 0; i < 3; i++) {
		status = __intel_cx0_write_once(port, lane, addr, data, committed);

		if (status == 0)
			return;
	}

	ERR("Port %d Write %04x failed after %d retries.\n", port, addr, i);
}

/**
 * @brief
 * This function performs read modify write operation by calling other helper function.
 * @param port - port
 * @param lane - lane number
 * @param addr - address to write
 * @param clear - bits to clear
 * @param set - bits to set
 * @param committed - flag to indicate if the write is committed or not
 */
void c10::__intel_cx0_rmw(u32 port, int lane, u16 addr, u8 clear, u8 set, bool committed)
{
	u8 old, val;

	old = __intel_cx0_read(port, lane, addr);
	val = (old & ~clear) | set;

	if (val != old)
		__intel_cx0_write(port, lane, addr, val, committed);
}

/**
 * @brief
 * This function performs read modify write operation on multiple lanes based on provided mask.
 * @param port - port number
 * @param lane_mask - lane mask
 * @param addr - address to read/write
 * @param clear - bits to clear
 * @param set - bits to set
 * @param committed - flag to indicate if the write is committed or not
 */
void c10::intel_cx0_rmw(u32 port, u8 lane_mask, u16 addr, u8 clear, u8 set, bool committed)
{
	u8 lane;

	for_each_cx0_lane_in_mask(lane_mask, lane)
		__intel_cx0_rmw(port, lane, addr, clear, set, committed);
}

/**
 * @brief
 * This function performs perform write on cx0 registers for multiple lanes.
 * @param port - port number
 * @param lane_mask - lane mask
 * @param addr - address to read/write
 * @param data - value to write
 * @param committed - flag to indicate if the write is committed or not
 */
void c10::intel_cx0_write(u32 port, u8 lane_mask, u16 addr, u8 data, bool committed)
{
	int lane;

	for_each_cx0_lane_in_mask(lane_mask, lane)
		__intel_cx0_write(port, lane, addr, data, committed);
}

