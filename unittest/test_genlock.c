#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vsyncalter.h>
#include <debug.h>
#include <math.h>
#include <getopt.h>
#include <stdbool.h>
#include "unity.h"
//#include "version.h"

void setUp(void) {

}

void tearDown(void) {

}

void test_get_phy_name() {

	char name[32];
	int i = 0;
	const char* device_str = find_first_dri_card();
	for (i = 0 ; i < 4; i++) {
		TEST_ASSERT_EQUAL_INT(0,vsync_lib_init(device_str, false));
		// Check if valid PHY
		if (get_phy_name(i, name, sizeof(name)) == false) {
			TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit());
			continue;
		}

		printf("Found %s\n", name);

		// It's a valid PHY on pipe.  Proceed with remaining tests.

		// Test 1: Null buffer
		TEST_ASSERT_FALSE(get_phy_name(i, NULL, 32));

		// Test 2: Zero size buffer
		TEST_ASSERT_FALSE(get_phy_name(i, name, 0));


		// Simulate uninitialized library
		TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit());

		TEST_ASSERT_FALSE(get_phy_name(i, name, sizeof(name)));

		// Simulate valid init but invalid pipe
		TEST_ASSERT_EQUAL_INT(0,vsync_lib_init(device_str, false));

		TEST_ASSERT_FALSE(get_phy_name(VSYNC_ALL_PIPES, name, sizeof(name)));

		// Simulate valid PHY setup

		TEST_ASSERT_TRUE(get_phy_name(i, name, sizeof(name)));
		TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit());

	}
}

void test_synchronize_vsync(void)
{
	int result, pipe;
	char name[32];
	const char* device_str = find_first_dri_card();
	for (pipe = 0 ; pipe < VSYNC_ALL_PIPES; pipe++) {

		TEST_ASSERT_EQUAL_INT(0,vsync_lib_init(device_str, false));
		set_log_level(LOG_LEVEL_INFO);
		// Check if valid PHY
		if (get_phy_name(pipe, name, sizeof(name)) == false) {
			TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit());
			continue;
		}

		// Case 1: Library not initialized
		TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit());
		// Case: PHY list is null
		result = synchronize_vsync(1.0, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_NOT_EQUAL(0, result);


		TEST_ASSERT_EQUAL_INT(0,vsync_lib_init(device_str, false));

		set_log_level(LOG_LEVEL_INFO);
		result = synchronize_vsync(0.5, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: With stepping
		result = synchronize_vsync(1.5, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: Too huge delta. e.g -50
		result = synchronize_vsync(-50.5, pipe, 0.01, 0.0, true, true);
		TEST_ASSERT_NOT_EQUAL(0, result);

		// Case: Large shift
		result = synchronize_vsync(1.0, pipe, 4.0, 0.1, false, false);
		TEST_ASSERT_NOT_EQUAL(0, result);

		// Case: Large shift1
		result = synchronize_vsync(1.0, pipe, 0.01, 4.0, false, false);
		TEST_ASSERT_NOT_EQUAL(0, result);

		// Case: All pipes
		result = synchronize_vsync(1.0, VSYNC_ALL_PIPES, 0.01, 0.02, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);
	}
}

void test_get_vsync(void)
{
	int result, pipe;
	char name[32];
	uint64_t vsync_array[VSYNC_MAX_TIMESTAMPS];
	const char* device_str = find_first_dri_card();
	for (pipe = 0 ; pipe < VSYNC_ALL_PIPES; pipe++) {
		TEST_ASSERT_EQUAL_INT(0,vsync_lib_init(device_str, false));

		// Check if valid PHY
		if (get_phy_name(pipe, name, sizeof(name)) == false) {
			TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit());
			continue;
		}
		// Case 1: Null array pointer
		result = get_vsync(device_str, NULL, 5, pipe);
		TEST_ASSERT_NOT_EQUAL(0, result);

		// Case 2: Zero size requested
		memset(vsync_array,0,sizeof(vsync_array));
		result = get_vsync(device_str, vsync_array, 0, pipe);
		TEST_ASSERT_NOT_EQUAL(0, result);
		TEST_ASSERT_EQUAL(0, vsync_array[0]);

				// Case 2: Zero size requested
		memset(vsync_array,0,sizeof(vsync_array));
		result = get_vsync(device_str, vsync_array, 5, pipe);
		TEST_ASSERT_EQUAL(0, result);
		TEST_ASSERT_EQUAL(0, vsync_array[5]);

		// Case 3: Invalid device string (simulate internal error or open failure)
		memset(vsync_array,0,sizeof(vsync_array));
		result = get_vsync("/dev/invalid", vsync_array, 5, pipe);
		TEST_ASSERT_NOT_EQUAL(0, result);
		TEST_ASSERT_EQUAL(0, vsync_array[5]);

		result = get_vsync("", vsync_array, 5, pipe);
		TEST_ASSERT_NOT_EQUAL(0, result);
		TEST_ASSERT_EQUAL(0, vsync_array[5]);

		result = get_vsync(NULL, vsync_array, 5, pipe);
		TEST_ASSERT_NOT_EQUAL(0, result);
		TEST_ASSERT_EQUAL(0, vsync_array[5]);

		// Case 4: Valid input, single vsync, default pipe
		result = get_vsync(device_str, vsync_array, VSYNC_MAX_TIMESTAMPS, pipe);
		TEST_ASSERT_EQUAL_INT(0, result);
		TEST_ASSERT_NOT_EQUAL(0, vsync_array[VSYNC_MAX_TIMESTAMPS]);
		TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit(device_str, false));
	}
}

void test_frequency_set(void) {
	const char* device_str = find_first_dri_card();
	double pll_clock = 0.0;
	double modified_pll_clock = 0.0;
	int ret = 0, pipe;

	set_log_level_str("debug");
	vsync_lib_init(device_str, false);
	set_log_level(LOG_LEVEL_ERROR);
	for (pipe = 0 ; pipe < VSYNC_ALL_PIPES; pipe++) {
		pll_clock = get_pll_clock(pipe);
		if (pll_clock <= 0.0)
			continue;
		modified_pll_clock = pll_clock + ((pll_clock * 0.01) / 100);
		printf("%lf,  %lf\n", pll_clock, modified_pll_clock);
		ret = set_pll_clock(modified_pll_clock, pipe, 0.01, VSYNC_DEFAULT_WAIT_IN_MS);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "Failed to set modified PLL clock");
		sleep(1);
		ret = set_pll_clock(pll_clock, pipe, 0.01, VSYNC_DEFAULT_WAIT_IN_MS);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "Failed to restore original PLL clock");
	}

	vsync_lib_uninit();
}

void test_get_vblank_interval(void)
{
	int pipe;
	char name[32];
	const char* device_str = find_first_dri_card();
	TEST_ASSERT_EQUAL_INT(0,vsync_lib_init(device_str, false));
	set_log_level(LOG_LEVEL_ERROR);
	// get_vblank_interval doesn't need vsync_lib_init.
	// vsync_lib_init is for get_phy_name to get enabled PHYs
	for (pipe = 0 ; pipe < VSYNC_ALL_PIPES; pipe++) {

		// Check if valid PHY
		if (get_phy_name(pipe, name, sizeof(name)) == false) {
			continue;
		}
		double interval = get_vblank_interval(device_str, pipe, 5);
		TEST_ASSERT_TRUE_MESSAGE(interval > 0.0, "Expected interval > 0.0 for valid input");

		// Case: Max stamps
		interval = get_vblank_interval(device_str, pipe, VSYNC_MAX_TIMESTAMPS);
		TEST_ASSERT_TRUE_MESSAGE(interval > 0.0, "Expected interval > 0.0 for valid input");

		// Case: 0 stamps
		interval = get_vblank_interval(device_str, pipe, 0);
		TEST_ASSERT_TRUE_MESSAGE(interval == 0.0, "Expected interval > 0.0 for valid input");

		// Case: 1 stamp
		interval = get_vblank_interval(device_str, pipe, 1);
		TEST_ASSERT_TRUE_MESSAGE(interval == 0.0, "Expected interval > 0.0 for valid input");

		// Case:  Invalid stamp count (higher)
		interval = get_vblank_interval(device_str, pipe, VSYNC_MAX_TIMESTAMPS+1);
		TEST_ASSERT_TRUE_MESSAGE(interval == 0.0, "Expected interval > 0.0 for valid input");

		// Case: Invalid device
		interval = get_vblank_interval("/dev/dri/invalid", pipe, VSYNC_MAX_TIMESTAMPS);
		TEST_ASSERT_TRUE_MESSAGE(interval == 0.0, "Expected interval > 0.0 for valid input");

		// Case: Empty device id
		interval = get_vblank_interval("", pipe, VSYNC_MAX_TIMESTAMPS);
		TEST_ASSERT_TRUE_MESSAGE(interval == 0.0, "Expected interval > 0.0 for valid input");

		// Case: NULL device Id
		interval = get_vblank_interval(NULL, pipe, VSYNC_MAX_TIMESTAMPS);
		TEST_ASSERT_TRUE_MESSAGE(interval == 0.0, "Expected interval > 0.0 for valid input");

	}

	TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit());
}

void test_drm_info(void)
{
	const char* device_str = find_first_dri_card();
 	TEST_ASSERT_EQUAL_INT(0,print_drm_info(device_str));
	TEST_ASSERT_EQUAL_INT(1,print_drm_info("/dev/dri/invalid"));
	TEST_ASSERT_EQUAL_INT(1,print_drm_info(""));
	TEST_ASSERT_EQUAL_INT(1,print_drm_info(NULL));
}

void test_m_n(void)
{
	int result, pipe;
	char name[32];
	const char* device_str = find_first_dri_card();
	TEST_ASSERT_EQUAL_INT(0,vsync_lib_init(device_str, true));
		set_log_level(LOG_LEVEL_INFO);
	for (pipe = 0 ; pipe < VSYNC_ALL_PIPES; pipe++) {


		// Check if valid PHY
		if (get_phy_name(pipe, name, sizeof(name)) == false) {
			continue;
		}

		if (strcmp(name, "M_N") != 0 ) {
			continue;
		}

		printf("Found %s\n", name);

		set_log_level(LOG_LEVEL_INFO);
		result = synchronize_vsync(0.1, pipe, 0.001, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);


		result = synchronize_vsync(0.1, pipe, 0.001, 0.01, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: no commit, no reset
		result = synchronize_vsync(0.1, pipe, 0.001, 0.1, false, false);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: Too huge delta. e.g -50
		result = synchronize_vsync(-50.5, pipe, 0.01, 0.0, true, true);
		TEST_ASSERT_NOT_EQUAL(0, result);

		// Case: Large shift
		result = synchronize_vsync(1.0, pipe, 4.0, 0.1, false, false);
		TEST_ASSERT_NOT_EQUAL(0, result);

		// Case: Large shift1
		result = synchronize_vsync(1.0, pipe, 0.01, 4.0, false, false);
		TEST_ASSERT_NOT_EQUAL(0, result);

		// Case: All pipes
		result = synchronize_vsync(1.0, VSYNC_ALL_PIPES, 0.01, 0.02, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);
	}

	// Case 1: Library not initialized
	TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit());
}


void test_logging(void)
{
	int pipe, result;
	char name[32];
	const char* device_str = find_first_dri_card();
	TEST_ASSERT_EQUAL_INT(0,vsync_lib_init(device_str, false));
	// get_vblank_interval doesn't need vsync_lib_init.
	// vsync_lib_init is for get_phy_name to get enabled PHYs
	for (pipe = 0 ; pipe < VSYNC_ALL_PIPES; pipe++) {
		// Check if valid PHY
		if (get_phy_name(pipe, name, sizeof(name)) == false) {
			continue;
		}
		set_log_mode("[UNITTEST]");
		set_log_level(LOG_LEVEL_NONE);
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case : Level ERROR
		set_log_level(LOG_LEVEL_ERROR);
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case : Level WARNING
		set_log_level(LOG_LEVEL_WARNING);
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case : Level INFO
		set_log_level(LOG_LEVEL_INFO);
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case : Level DEBUG
		set_log_level(LOG_LEVEL_DEBUG);
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case : Level TRACE
		set_log_level(LOG_LEVEL_TRACE);
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case : Level NONE
		set_log_level(LOG_LEVEL_NONE);
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case : Invalid Level
		set_log_level(16);
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: str Level NULL
		set_log_level_str(NULL);
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: str Level error
		set_log_level_str("error");
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: str Level warning
		set_log_level_str("warning");
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: str Level info
		set_log_level_str("info");
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: str Level debug
		set_log_level_str("debug");
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);

		// Case: str Level trace
		set_log_level_str("trace");
		result = synchronize_vsync(0.05, pipe, 0.01, 0.1, true, true);
		TEST_ASSERT_EQUAL_INT(0, result);
	}

	TEST_ASSERT_EQUAL_INT(0,vsync_lib_uninit());

}


int main(int argc, char **argv) {
	UNITY_BEGIN();

	// Check for optional flag
	int run_mn_test = 0;
	if (argc > 1 && strcmp(argv[1], "--run-mn-test") == 0) {
		run_mn_test = 1;
	}

	RUN_TEST(test_frequency_set);
	RUN_TEST(test_get_phy_name);
	RUN_TEST(test_synchronize_vsync);
	RUN_TEST(test_get_vsync);
	RUN_TEST(test_get_vblank_interval);
	RUN_TEST(test_drm_info);
	RUN_TEST(test_logging);

	if (run_mn_test) {
		RUN_TEST(test_m_n);
	}

	return UNITY_END();
}
