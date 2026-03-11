/*
 * Copyright © 2024-2026 Intel Corporation
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

#include "phy.h"
#include "debug.h"
#include "common.h"
#include "mmio.h"
#include "process_platform.h"
#include "system_platform.h"
#include <cmath>
#include <cstdint>
#include <cstring>

// Nanoseconds per second conversion factor
static const long NANOSECONDS_PER_SECOND = 1000000000L;
constexpr long MICROSECONDS_TO_MILLISECONDS = 1000;
constexpr double PERCENT_PRECISION_FACTOR = 10000.0;
constexpr double PERCENT_TO_FRACTION = 100.0;

// Global timestamp variables for performance measurement
static os_timespec g_start_time;
static os_timespec g_end_time;

/**
* @brief
* This function resets the Combo Phy MMIO registers to their
* original value. It gets executed whenever a timer expires. We program MMIO
* registers of the PHY in this function becase we have waited for a certain
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
void phys::reset_phy_regs(int sig UNUSED, siginfo_t* si, void* /*context*/)
{
	phys* phy = (phys*)si->si_value.sival_ptr;
	if (!phy) {
		ERR("Received null pointer in signal handler.");
		return;
	}

	DBG("Signal handled for timer expiration.\n");
	phy->reset_phy_regs();
}

/**
* @brief
* Invokes the concrete implementation of `program_mmio` in the respective
* class to restore PHY registers to their original state.
* @param None
* @return void
*/
void phys::reset_phy_regs()
{
	TRACING();

	// Delete the timer before resetting the PLL clock.
	// The set_pll_clock function may use step mode, which takes time to complete.
	// Without deleting the timer, it might expire and trigger again during this delay.
	timer_delete(get_timer());

	set_pll_clock(pll_freq_mod, pll_freq_orig, used_shift, _wait_between_steps);

	// set_pll_clock is expected to restore the original value using pll_freq_orig (double).
	// However, due to precision loss when converting from double to divider factors,
	// we explicitly set the registers back to the original value as a safeguard.
	program_mmio(false);

	os_clock_gettime(OS_CLOCK_MONOTONIC, &g_end_time);
	// Calculate elapsed time in nanoseconds
	long elapsed_ns = (g_end_time.tv_sec - g_start_time.tv_sec) * NANOSECONDS_PER_SECOND +
						(g_end_time.tv_nsec - g_start_time.tv_nsec);
	// Convert to seconds with nanosecond precision
	double elapsed_time_sec = (double)elapsed_ns / (double)NANOSECONDS_PER_SECOND;
	INFO("\t[Total Elapsed time: %.9f sec]\n", elapsed_time_sec);

	done = true;
}

/**
* @brief
* This function programs Combo phys on the system
* @param time_diff - This is the time difference in between the primary and the
* secondary systems in ms. If master is ahead of the slave , then the time
* difference is a positive number otherwise negative.
* @param shift - Fraction value to be used during the calculation.
* @param shift2 - Fraction value to be used extended calculation (step by step).
* @param step_threshold - step_threshold  Delta threshold in microseconds to trigger stepping mode
* @param wait_between_steps - Wait in milliseconds between steps
* @param reset - This is a boolean value. If true, then we will reset the
* registers to their original values after the shift has been applied.
* @param commit - This is a boolean value. If false, then we will not program
* the PHY registers. This is useful for debugging purposes.
* @return int, 0 - success, non zero - failure
*/
int phys::program_phy(double time_diff, double shift, double shift2, int step_threshold,
						int wait_between_steps, bool reset, bool commit)
{
	TRACING();
	ddi_sel* dsel = get_ds();
	if (!dsel) {
		ERR("Invalid ddi_sel\n");
		return 1;
	}

	// If the delta duration exceeds a defined threshold and shift2 is provided,
	// the implementation uses shift2 (a larger shift value) to more rapidly reduce the drift.
	// However, since applying a large shift directly can cause the monitor to blank,
	// the algorithm internally breaks shift2 into smaller increments (using shift) and
	// applies them step by step to safely reach the desired frequency.

	// Once the timer expires, the same process is reversed to gradually return to the
	// original frequency. If shift2 is set to 0, only shift is used, without any internal stepping.
	// This results in slower convergence toward the desired timestamp.

	// The use of shift2 is especially helpful during the initial run, when vblank timestamps
	// from the display may be significantly misaligned. It helps quickly minimize the drift,
	// after which subsequent synchronization steps use shift for fine-tuning.

	// Ensure shift is non-negative before proceeding
	shift = fabs(shift);

	// Use larger shift for desired frequency calculation if time_diff is greater than 1 ms
	double _shift = ((shift2 && (fabs(time_diff) * MICROSECONDS_TO_MILLISECONDS) >= step_threshold) ? shift2 : shift);

	int steps = CALC_STEPS_TO_SYNC(time_diff, _shift);
	DBG("steps are %d - (Step threshold = %d us)\n", steps, step_threshold);
	read_registers();

	double pll_clock = calculate_pll_clock();
	pll_freq_orig = pll_clock;
	// Computes `new_pll_clock` by applying a percentage shift to the original `pll_clock`.
	// The direction of adjustment is determined by `time_diff`:
	// - A **positive `time_diff`** indicates that timestamps should drift forward, requiring a **decrease** in PLL frequency.
	//   This increases the vblank time period, effectively delaying the timestamps.
	// - A **negative `time_diff`** indicates that timestamps should drift backward, requiring an **increase** in PLL frequency.
	double shift_impact = _shift * pll_clock / PERCENT_TO_FRACTION;
	double direction_adjustment = (time_diff > 0) ? -1 : 1;
	double delta = shift_impact * direction_adjustment;

	double new_pll_clock = pll_clock + delta;
	pll_freq_mod = new_pll_clock;
	used_shift = shift;
	INFO("Changes to be made\n");
	INFO("\tpll clock: %lf -> %lf\n", pll_clock, new_pll_clock);

	// If commit is false, the function logs the calculated values above
	// but does not update the hardware registers.
	if (!commit || steps == 0) {
		WARNING("Registers not updated\n");
		return 0;
	}

	// Captures current timestamp and stores it globally for elapsed time measurement
	// This capturing is for debugging and troubleshooting.
	os_clock_gettime(OS_CLOCK_MONOTONIC, &g_start_time);

	if (set_pll_clock(pll_clock, new_pll_clock, shift, wait_between_steps, commit) != 0) {
		ERR("Failed to set PLL clock.\n");
		return 1;
	}

	if (!commit || dbg_lvl >= LOG_LEVEL_DEBUG) {
		print_registers();
	}

	if (reset) {
		// For whichever PHY we find, let's set the done flag to 0 so that we can later
		// have a timer for it to reset the default values back in their registers
		done = false;
		_wait_between_steps = wait_between_steps;
		make_timer((long)steps, this, reset_phy_regs);
	}

	return 0;
}

/**
* @brief
*	This function waits until the Combo programming is
*	finished. There is a timer for which time the new values will remain in
*	effect. After that timer expires, the original values will be restored.
* @param t - The timer which needs to be deleted
* @return void
*/
void phys::wait_until_done(void)
{
	TRACING();

	// Wait to write back the original value.  Exit loop if Ctrl+C pressed
	while (!done && !lib_client_done) {
		os_usleep(MICROSECONDS_TO_MILLISECONDS);
	}

	// Restore original values in case of app termination
	if (lib_client_done) {
		reset_phy_regs();
	}


}

/**
 * @brief
 * This function is a wrapper function and calls set_pll_clock.
 * @param target_pll_clock - The desired PLL clock
 * @param shift - Fraction value to be used during the calculation if difference is large.
 * @param wait_between_steps - Wait in milliseconds to be applied between steps after
 * each time programming the registers.
 * @return 0 - success, non zero - failure
 */
int phys::set_pll_clock(double target_pll_clock, double shift, uint32_t wait_between_steps)
{
	if (target_pll_clock <= 0 || shift < 0) {
		ERR("Invalid input to set_pll_clock(). Target: %f, Shift: %f\n", target_pll_clock, shift);
		return 1;
	}

	read_registers();

	double current_pll_clock = calculate_pll_clock();
	return set_pll_clock(current_pll_clock, target_pll_clock, shift, wait_between_steps);
}

/**
 * @brief
 * This function sets the PLL clock for the given pipe.
 * This is different from the program_phy method.
 * @param current_pll_clock - The current PLL clock
 * @param target_pll_clock - The desired PLL clock
 * @param shift - Fraction value to be used during the calculation if difference is large.
 * @param wait_between_steps - Wait in milliseconds to be applied between steps after
 * each time programming the registers.
 * @param commit - commit values to HW registers
 * @return 0 - success, non zero - failure
 */
int phys::set_pll_clock(double current_pll_clock, double target_pll_clock, double shift,
								uint32_t wait_between_steps, bool commit)
{
	TRACING();

	DBG("Initial PLL clock: %f\n", current_pll_clock);
	DBG("Desired PLL clock: %f\n", target_pll_clock);
	DBG("Shift: %0.5f\n", shift);

	if (current_pll_clock <= 0 || target_pll_clock <= 0 || shift < 0) {
		ERR("Invalid input to set_pll_clock().\n");
		return 1;
	}
	// Calculate the percentage difference between current and desired clocks
	double percent_diff = (fabs(target_pll_clock - current_pll_clock) / current_pll_clock) * 100;
	// Use back upto 4 decimal places
	percent_diff = round(percent_diff * PERCENT_PRECISION_FACTOR) / PERCENT_PRECISION_FACTOR;
	// Determine the direction of the change
	const double step_direction = (target_pll_clock > current_pll_clock) ? 1.0 : -1.0;

	// Initialize steps to 1 for the case where the change is within shift limits
	int steps = 1;
	double step_size = target_pll_clock - current_pll_clock; // Default step size is the total change

	// Calculate steps and step size only if the percentage difference exceeds the shift limit
	if (percent_diff > shift) {
		INFO("Large PLL clock change detected. Applying in steps.\n");
		// Calculate the total change needed
		const double total_change = target_pll_clock - current_pll_clock;

		// Calculate the ideal step size based on the shift percentage
		step_size = current_pll_clock * shift / 100.0 * step_direction;

		// Calculate the number of steps, rounding up to ensure the target is reached or exceeded
		steps = ceil(fabs(total_change / step_size));

		// If there are multiple steps, adjust step_size to distribute the change evenly
		if (steps > 1) {
			step_size = total_change / steps;
		}
	}

	double intermediate_pll_clock = current_pll_clock;
	DBG("Adjusting PLL clock from %f to %f in %d steps - Wait time between steps = %d ms\n", current_pll_clock, target_pll_clock, steps, wait_between_steps);
	for (int i = 0; i < steps && !lib_client_done; i++) {
		// For the last step, set the intermediate clock to the desired clock
		if (i == steps - 1) {
			intermediate_pll_clock = target_pll_clock;
		}
		else {
			intermediate_pll_clock += step_size;
		}

		DBG("Intermediate Steps\n");
		DBG("\tStep: %d of %d\n", i + 1, steps);
		DBG("\tintermediate pll clock: %f\n", intermediate_pll_clock);

		if (calculate_feedback_dividers(intermediate_pll_clock) != 0) {
			ERR("Failed to calculate feedback dividers for clock %f\n", intermediate_pll_clock);
			return 1;
		}

		if (commit && program_mmio(true) != 0) {
			ERR("Failed to program MMIO during PLL adjustment step %d\n", i + 1);
			return 1;
		}

		// Wait is needed otherwise changing registers quickly will create trearing on screen.
		// Wait only if stepping multiple times
		if (steps > 1) {
			os_usleep(wait_between_steps * MICROSECONDS_TO_MILLISECONDS); // Convert to microseconds
		}
	}

	return 0;
}

/**
 * @brief
 * This function returns pll clock on current PHY.
 *
 * @return double - The calculated PLL clock
 */
double phys::get_pll_clock(void)
{
	read_registers();
	return calculate_pll_clock();
}
