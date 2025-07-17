#include <signal.h>
#include <errno.h>
#include <time.h>
#include "cx0_helper.h"
#include "mmio.h"
#include "debug.h"
#include "common.h"

namespace cx0 {

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
int __intel_wait_for_register_fw(
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
int __intel_wait_for_register(
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
int intel_wait_for_register(u32 reg,
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
int __intel_de_wait_for_register(u32 reg,
				 u32 mask, u32 value,
				 u32 *out_value)
{
	int out = __intel_wait_for_register(reg, mask, value, out_value);
	if(out_value) {
		TRACE("Wait: reg: 0x%X, mask = 0x%X, value = 0x%X, out_value = 0x%X\n", reg, mask, value, *out_value);
	} else {
		TRACE("Wait: reg: 0x%X, mask = 0x%X, value = 0x%X\n", reg, mask, value);
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
int intel_de_wait_for_register(u32 reg, u32 mask, u32 value, unsigned int timeout)
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
u32 intel_uncore_rmw(u32 reg, u32 clear, u32 set)
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
u32 intel_de_rmw(u32 reg, u32 clear, u32 set)
{
	TRACE("RMW: reg: 0x%X, clear = 0x%X, set = 0x%X\n", reg, clear, set);
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
int intel_de_wait_for_clear(int reg, u32 mask, unsigned int timeout)
{
	return intel_de_wait_for_register(reg, mask, 0, timeout);
}

/**
 * @brief
 * This function reads the value from the register.
 * @param reg - The address or identifier of the register to be read.
 * @return u32 -
 */
u32 intel_de_read(u32 reg)
{
	u32 val = READ_OFFSET_DWORD(reg);
	TRACE("%s: %d reg: 0x%X, value = 0x%X\n", __FUNCTION__, __LINE__, reg, val);
	return val;
}

/**
 * @brief
 * This function writes the value to the register.
 * @param reg - The address or identifier of the register to be written.
 * @param val - The value to be written to the register.
 */
void intel_de_write(u32 reg, u32 val)
{
	TRACE("Write: reg: 0x%X, val = 0x%X\n", reg, val);
	WRITE_OFFSET_DWORD(reg, val);
}

/**
 * @brief
 * This function clears response ready flag by calling other helper function.
 * @param port - port number
 * @param lane - lane number
 */
void intel_clear_response_ready_flag(u32 port, int lane)
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
void intel_cx0_bus_reset(int port, int lane)
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
int intel_cx0_wait_for_ack(u32 port, int command, int lane, u32 *val)
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

unsigned int ilog2(unsigned int x) {
	unsigned int log = 0;
	while (x >>= 1) { // Keep shifting x right until it's 0, count how many shifts are necessary
		log++;
	}
	return log;
}

static int lane_mask_to_lane(u8 lane_mask)
{
	return ilog2(lane_mask);
}

u16 intel_c20_sram_read(u32 port, int lane, u16 addr)
{
	//struct drm_i915_private* i915 = to_i915(encoder->base.dev);
	u16 val = 0;

	//assert_dc_off(i915);

	intel_cx0_write(port, lane, PHY_C20_RD_ADDRESS_H, addr >> 8, 0);
	intel_cx0_write(port, lane, PHY_C20_RD_ADDRESS_L, addr & 0xff, 1);

	val = intel_cx0_read(port, lane, PHY_C20_RD_DATA_H);
	val <<= 8;
	val |= intel_cx0_read(port, lane, PHY_C20_RD_DATA_L);

	return val;
}

void intel_c20_sram_write(u32 port,
    int lane, u16 addr, u16 data)
{
    intel_cx0_write(port, lane, PHY_C20_WR_ADDRESS_H, addr >> 8, 0);
    intel_cx0_write(port, lane, PHY_C20_WR_ADDRESS_L, addr & 0xff, 0);

    intel_cx0_write(port, lane, PHY_C20_WR_DATA_H, data >> 8, 0);
    intel_cx0_write(port, lane, PHY_C20_WR_DATA_L, data & 0xff, 1);
}

u8 intel_cx0_read(u32 port, u8 lane_mask, u16 addr)
{
	int lane = lane_mask_to_lane(lane_mask);
	return __intel_cx0_read(port, lane, addr);
}

/**
 * @brief
 * This function reads the value from c10 registers using bus mechanism introduced in MTL/PTL.
 * @param port - target port
 * @param lane - target lane
 * @param addr - address
 * @return int
 */
int __intel_cx0_read_once(u32 port, int lane, u16 addr)
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
u8 __intel_cx0_read(u32 port, int lane, u16 addr)
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
int __intel_cx0_write_once(u32 port, int lane, u16 addr, u8 data, bool committed)
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
void __intel_cx0_write(u32 port, int lane, u16 addr, u8 data, bool committed)
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
void __intel_cx0_rmw(u32 port, int lane, u16 addr, u8 clear, u8 set, bool committed)
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
void intel_cx0_rmw(u32 port, u8 lane_mask, u16 addr, u8 clear, u8 set, bool committed)
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
void intel_cx0_write(u32 port, u8 lane_mask, u16 addr, u8 data, bool committed)
{
	int lane;

	for_each_cx0_lane_in_mask(lane_mask, lane)
		__intel_cx0_write(port, lane, addr, data, committed);
}

} // namespace cx0
