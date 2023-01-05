INTEL CONFIDENTIAL
Copyright (C) 2022 Intel Corporation
This software and the related documents are Intel copyrighted materials, and your use of them is governed by the express license under which they were provided to you ("License"). Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties, other than those that are expressly stated in the License.

This program allows a system to alter their vertical sync firing times.

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
