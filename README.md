 Copyright (C) 2023 Intel Corporation
 SPDX-License-Identifier: MIT

This program allows a system to alter their vertical sync firing times.
This is sample code only and is only supported on 11th Gen Core. 

# Prerequisites
The build system assumes the following, prior to executing the build steps:
1) The system has an [Ubuntu OS](https://ubuntu.com/tutorials/install-ubuntu-desktop#1-overview)
1) Libraries installed: `sudo apt install -y git build-essential make`

# Building steps:
1) Git clone this repository 
1) `sudo apt install libdrm-dev libpciaccess-dev`
    1) If the directory /usr/include/drm does not exist, you may need to also add `sudo apt install -y linux-libc-dev`
1) Type `make release` from the main directory. It compiles everything and creates a 
   release folder which can then be copied to the target systems.

Building of this program has been succesfully tested on both Ubuntu 20 and Fedora 30.

# Installing and running the programs:
1) On the system, go to the directory where these files exist and set environment variable:
```console
export LD_LIBRARY_PATH=`pwd`
``````
2) On the primary system, run it as follows:
	```console
   ./synctest 
   ```

(Note by default this is set to move vsync by 1.0 ms.)

# Helpful libraries to have installed
If you are doing debug, it helps to have the following libraries installed:
```
apt install -y intel-gpu-tools edid-decode
```
