#ifndef _PHY_H
#define _PHY_H

#include "common.h"

class phys {
	protected:
		int done;
		int phy_type;
	private:
		bool init;
		int pipe;
		timer_t timer_id;
		ddi_sel *m_ds;
		double pll_freq_orig;
		double pll_freq_mod;
		double used_shift;
		int _wait_between_steps;
	public:
		phys(int _pipe) : done(0), phy_type(-1), init(false), pipe(_pipe), timer_id(0),
					m_ds(NULL), pll_freq_orig(0.0), pll_freq_mod(0.0), used_shift(0.0),
					_wait_between_steps(0) {}
		virtual ~phys() { }
		bool is_init() { return init; }
		void set_init(bool i) { init = i; }
		int get_pipe() { return pipe; }
		void set_pipe(int p) { pipe = p; }
		int get_phy_type() { return phy_type; }
		timer_t get_timer() { return timer_id; }
		void set_ds(ddi_sel *ds) { m_ds = ds; }
		ddi_sel *get_ds() { return m_ds; }
		int make_timer(long expire_ms, void *user_ptr, reset_func reset);
		static void reset_phy_regs(int sig, siginfo_t *si, void *uc);
		void reset_phy_regs();
		int program_phy(double time_diff, double shift, double shift2, int step_threshold,
							int wait_between_steps, bool reset, bool commit);
		virtual void wait_until_done();
		int set_pll_clock(double target_pll_clock, double shift, uint32_t wait_between_steps);
		int set_pll_clock(double current_pll_clock, double target_pll_clock, double shift,
				uint32_t wait_between_steps, bool commit = true);
		double get_pll_clock(void);
		virtual int program_mmio(int mod)= 0;
		virtual double calculate_pll_clock() = 0;
		virtual int calculate_feedback_dividers(double pll_freq) = 0;
		virtual void print_registers() = 0;
		virtual void read_registers() = 0;
	};

	#endif // _PHY_H
