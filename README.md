 Copyright (C) 2023 Intel Corporation
 SPDX-License-Identifier: MIT

This program allows a system to alter their vertical sync firing times.
This is sample code only and is only supported on 11th Gen Core. 

Building steps:
1) sudo apt install libdrm-dev libpciaccess-dev
1) Type 'make release' from the main directory. It compiles everything and creates a 
   release folder which can then be copied to the target systems.

Building of this program has been succesfully tested on both Ubuntu 20 and Fedora 30.

Installing and running the programs:
1) On the system, go to the directory where these files exist and set environment variable:
export LD_LIBRARY_PATH=.
2) On the primary system, run it as follows:
	./synctest 
(Note by default this is set to move vsync by 1.0 ms.)
