// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: MIT

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vsyncalter.h>
#include <debug.h>
#include <math.h>

using namespace std;

/*******************************************************************************
 * Description
 *	main - This is the main function
 * Parameters
 *	int argc - The number of command line arguments
 *	char *argv[] - Each command line argument in an array
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int main(int argc, char *argv[])
{
	int ret = 0;

	if(vsync_lib_init()) {
		ERR("Couldn't initialize vsync lib\n");
		return 1;
	}

	synchronize_vsync(1.0);

	vsync_lib_uninit();
	return ret;
}
