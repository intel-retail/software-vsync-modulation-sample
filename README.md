 Copyright (C) 2023 Intel Corporation
 SPDX-License-Identifier: MIT

This program allows a system to alter their vertical sync firing times.
This is sample code only and is only supported on 11th Gen Core. Please note that this document was written based on Ubuntu

# Prerequisites
The build system assumes the following, prior to executing the build steps:
1) The system has an [Ubuntu OS](https://ubuntu.com/tutorials/install-ubuntu-desktop#1-overview)
1) Libraries installed: `sudo apt install -y git build-essential make`
1) [Disable secure boot](https://wiki.ubuntu.com/UEFI/SecureBoot/DKMS)

# Building steps:
1) Git clone this repository 
1) `sudo apt install libdrm-dev libpciaccess-dev`
    1) If the directory /usr/include/drm does not exist, you may need to also add `sudo apt install -y linux-libc-dev`
1) Type `make release` from the main directory. It compiles everything and creates a 
   release folder which can then be copied to the target systems.

# Notes
* By default this is set to move vsync by 1.0 ms. in [synctest/synctest.cpp](./synctest/synctest.cpp)

# Debug
You can turn on debug printing automatically by building with `make debug`
There is a dbg_lvl flag which can be set to INFO, DBG, ERR etc.

**sample output:**
```console
./synctest

[DBG] Device id is 0xA780
[DBG] 0x60400 = 0xA0030011
[DBG] ddi_select = 0x4
[DBG] Detected a Combo phy on pipe 1
[DBG] 0x164280 = 0xE07410
[DBG] DPLL num = 0x0
[DBG] 0x61400 = 0xAA110006
[DBG] ddi_select = 0x5
[DBG] Detected a Combo phy on pipe 2
[DBG] 0x164280 = 0xE07410
[DBG] DPLL num = 0x1
[DBG] 0x62400 = 0x0
[DBG] Pipe 3 is turned off
[DBG] 0x63400 = 0x0
[DBG] Pipe 4 is turned off
[DBG] steps are 1000
[DBG] OLD VALUES
 cfgcr0 [0x164284] =	 0x1001D0
 cfgcr1 [0x164288] =	 0x448
[DBG] old pll_freq 	 594.000000
[DBG] new_pll_freq 	 594.594000
[DBG] old dco_clock 	 8910.000000
[DBG] new dco_clock 	 8918.910000
[DBG] old fbdivfrac 	 0x400
[DBG] old ro_div_frac 	 0x8000
[DBG] old fbdivint 	 0x1D0
[DBG] old ro_div_int 	 0x3A
[DBG] new fbdivfrac 	 0x21B3
[DBG] new ro_div_frac 	 0x43666
[DBG] NEW VALUES
 cfgcr0 [0x164284] =	 0x86CDD0
[DBG] timer done
[DBG] DEFAULT VALUES
 cfgcr0 [0x164284] =	 0x1001D0
 cfgcr1 [0x164288] =	 0x448
[DBG] steps are 1000
[DBG] OLD VALUES
 cfgcr0 [0x16428C] =	 0xE001A5
 cfgcr1 [0x164290] =	 0x48
[DBG] old pll_freq 	 540.000000
[DBG] new_pll_freq 	 540.540000
[DBG] old dco_clock 	 8100.000000
[DBG] new dco_clock 	 8108.100000
[DBG] old fbdivfrac 	 0x3800
[DBG] old ro_div_frac 	 0x2F0000
[DBG] old fbdivint 	 0x1A6
[DBG] old ro_div_int 	 0x34
[DBG] new fbdivfrac 	 0x11300
[DBG] new ro_div_frac 	 0x226001
[DBG] NEW VALUES
 cfgcr0 [0x16428C] =	 0x44C01A6
[DBG] timer done
[DBG] DEFAULT VALUES
 cfgcr0 [0x16428C] =	 0xE001A5
 cfgcr1 [0x164290] =	 0x48
```

## Debug Tools
If you are doing debug, it helps to have the following libraries installed:
```
apt install -y intel-gpu-tools edid-decode
```

### Debug Tool Usage
If you are encountering unexpected behavior that requires additional debug, the tools installed can assist with 
narrowing the potential problem.

**[intel-gpu-tools](https://cgit.freedesktop.org/xorg/app/intel-gpu-tools/)**
Provides tools to read and write to registers, it must be run as the root user. Example:

```console
intel_reg dump --all > /tmp/reg_dump.txt

# output excerpt
                    GEN6_RP_CONTROL (0x0000a024): 0x00000400
Gen6	disabled
                      GEN6_RPNSWREQ (0x0000a008): 0x150a8000
               GEN6_RP_DOWN_TIMEOUT (0x0000a010): 0x00000000
           GEN6_RP_INTERRUPT_LIMITS (0x0000a014): 0x00000000
               GEN6_RP_UP_THRESHOLD (0x0000a02c): 0x00000000
                      GEN6_RP_UP_EI (0x0000a068): 0x00000000
                    GEN6_RP_DOWN_EI (0x0000a06c): 0x00000000
             GEN6_RP_IDLE_HYSTERSIS (0x0000a070): 0x00000000
                      GEN6_RC_STATE (0x0000a094): 0x00040000
                    GEN6_RC_CONTROL (0x0000a090): 0x00040000
           GEN6_RC1_WAKE_RATE_LIMIT (0x0000a098): 0x00000000
           GEN6_RC6_WAKE_RATE_LIMIT (0x0000a09c): 0x00000000
        GEN6_RC_EVALUATION_INTERVAL (0x0000a0a8): 0x00000000
             GEN6_RC_IDLE_HYSTERSIS (0x0000a0ac): 0x00000019
                      GEN6_RC_SLEEP (0x0000a0b0): 0x00000000
                GEN6_RC1e_THRESHOLD (0x0000a0b4): 0x00000000
                 GEN6_RC6_THRESHOLD (0x0000a0b8): 0x00000000
...

intel_reg read 0x00062400
                PIPE_DDI_FUNC_CTL_C (0x00062400): 0x00000000 (disabled, no port, HDMI, 8 bpc, -VSync, -HSync, EDP A ON, x1)
```

**[edid-decode](https://manpages.debian.org/unstable/edid-decode/edid-decode.1.en.html)**
is a tool used to parse the Extended Display Identification Data (EDID) from monitors and display devices. EDID contains metadata about the display's capabilities, such as supported resolutions, manufacturer, serial number, and other attributes.

To run edid-decode, you typically need to provide it with a binary EDID file or data. Sample usage:
```console
cat /sys/class/drm/card0-HDMI-A-1/edid > edid.bin

# run edid-decode to parse and display the information
edid-decode edid.bin

# sample output
Extracted contents:
header:          00 ff ff ff ff ff ff 00
serial number:   10 ac 64 a4 4c 30 30 32
version:         01 03
basic params:    80 34 20 78 2e
chroma info:     c5 c4 a3 57 4a 9c 25 12 50 54
established:     bf ef 80
standard:        71 4f 81 00 81 40 81 80 95 00 a9 40 b3 00 01 01
descriptor 1:    4d d0 00 a0 f8 70 3e 80 30 20 35 00 55 50 21 00 00 1a
descriptor 2:    02 3a 80 18 71 38 2d 40 58 2c 45 00 55 50 21 00 00 1e
descriptor 3:    00 00 00 ff 00 43 32 30 30 38 30 4e 50 30 0a 20 20 20
descriptor 4:    00 00 00 fd 00 32 4b 1e 53 11 00 0a 20 20 20 20 20 20
extensions:      01
checksum:        1c

Manufacturer: DEL Model a464 Serial Number 808909324
Made week 12 of 2008
EDID version: 1.3
Digital display
Maximum image size: 52 cm x 32 cm
Gamma: 2.20
DPMS levels: Standby Suspend Off
RGB color display
First detailed timing is preferred timing
Display x,y Chromaticity:
  Red:   0.6396, 0.3300
  Green: 0.2998, 0.5996
  Blue:  0.1503, 0.0595
  White: 0.3134, 0.3291
Established timings supported:
  720x400@70Hz
  640x480@60Hz
  640x480@67Hz
  640x480@72Hz
  640x480@75Hz
  800x600@56Hz
  800x600@60Hz
  ...
Standard timings supported:
  1152x864@75Hz
  1280x800@60Hz
  1280x960@60Hz
  ...
Detailed mode: Clock 148.500 MHz, 509 mm x 286 mm
               1920 2008 2052 2200 hborder 0
               1080 1084 1089 
```

# Generating Doxygen documents

Please install doxygen and graphviz packages before generating Doxygen documents:
	```sudo apt install doxygen graphviz```
1. Type ```make doxygen``` from the main directory. It will genarte SW Genlock doxygen documents
	to output/doxygen folder.
2. Open output/doxygen/html/index.html with a web-browser

# Build Steps
1) On the system, go to the directory where these files exist and set environment variable:
```console
export LD_LIBRARY_PATH=`pwd`/lib
``````
2) On the primary system, run it as follows:
	```console
   ./synctest 
   ```

Building of this program has been successfully tested on both Ubuntu 2x and Fedora 30.<br>

## Preparing two systems for PTP communication
In order to prepare two systems, please follow these steps:<br>
1. There should be a network cable directly connecting the ethernet ports of the two
systems.
2. Apply __[the monotonic timstamp patch](./resources/0001-Revert-drm-vblank-remove-drm_timestamp_monotonic-par.patch)__
to Linux kernel i915 driver which allows it to provide vsync timestamps in real time
instead of the default monotonic time.<br>
Note that the monotonic timestamp patch is generated based on Linux v6.4 and has been tested on Linux v6.4 and v6.5.<br>
Please follow the steps to disable monotonic timestamp after installing the local built Linux image on a target.<br>
	1. Add ```drm.timestamp_monotonic=0``` option to __GRUB_CMDLINE_LINUX__ in /etc/default/grub
	2. Update __GRUB_DEFAULT__ in /etc/default/grub to address the local built Linux image
	3. Run ```update-grub``` to apply the change
	4. Reboot a target
3. Turn off NTP time synchronization service by using this command:
	```timedatectl set-ntp no```
4. Run ptp sync on both the systems as root user.<br>
	On the primary system, run the following command:
	```ptp4l -i enp176s0 -m -f gPTP.cfg```<br>
	On the secondary system, run the following command:
	```ptp4l -i enp176s0 -m -f gPTP.cfg -s```

	There must be a __[gPTP.cfg](./resources/gPTP.cfg)__ file present in the same directory from where you run the
	above two commands. The contents of this file should look like this:<br>

	```shell
	# 802.1AS example configuration containing those attributes which
	# differ from the defaults.  See the file, default.cfg, for the
	# complete list of available options.
	[global]
	gmCapable		1
	priority1		248
	priority2		248
	logAnnounceInterval	0
	logSyncInterval		-3
	syncReceiptTimeout	3
	neighborPropDelayThresh 15000
	min_neighbor_prop_delay	-20000000
	assume_two_step		1
	path_trace_enabled	1
	follow_up_info		1
	transportSpecific	0x1
	network_transport	L2
	delay_mechanism		P2P
	```

	The output on the primary system after running the above command should look
	like this:<br>
	```console
	ptp4l[13416.983]: selected /dev/ptp1 as PTP clock
	ptp4l[13417.002]: port 1: INITIALIZING to LISTENING on INIT_COMPLETE
	ptp4l[13417.003]: port 0: INITIALIZING to LISTENING on INIT_COMPLETE
	ptp4l[13420.445]: port 1: LISTENING to MASTER on ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES
	ptp4l[13420.445]: selected local clock 844709.fffe.04f07f as best master
	ptp4l[13420.445]: assuming the grand master role
	```

	The output on the secondary system after running the above command should
	look like this:<br>
	```console
	ptp4l[14816.313]: selected /dev/ptp1 as PTP clock
	ptp4l[14816.328]: port 1: INITIALIZING to LISTENING on INIT_COMPLETE
	ptp4l[14816.328]: port 0: INITIALIZING to LISTENING on INIT_COMPLETE
	ptp4l[14820.220]: port 1: new foreign master 844709.fffe.04f07f-1
	ptp4l[14820.250]: selected local clock 844709.fffe.04eb0e as best master
	ptp4l[14822.220]: selected best master clock 844709.fffe.04f07f
	ptp4l[14822.220]: port 1: LISTENING to UNCALIBRATED on RS_SLAVE
	ptp4l[14822.650]: port 1: UNCALIBRATED to SLAVE on MASTER_CLOCK_SELECTED
	ptp4l[14823.275]: rms 158899 max 317730 freq -11742 +/- 3075 delay    14 +/-   0
	ptp4l[14824.276]: rms  820 max 1257 freq  -9067 +/- 1113 delay    14 +/-   0
	ptp4l[14825.277]: rms 1366 max 1436 freq  -6582 +/- 359 delay    13 +/-   0
	ptp4l[14826.278]: rms  866 max 1148 freq  -6099 +/-  34 delay    13 +/-   0
	ptp4l[14827.279]: rms  280 max  465 freq  -6364 +/- 102 delay    12 +/-   0
	ptp4l[14828.280]: rms   52 max   80 freq  -6666 +/-  65 delay    12 +/-   0
	ptp4l[14829.280]: rms   82 max   88 freq  -6805 +/-  21 delay    12 +/-   0
	ptp4l[14830.281]: rms   50 max   69 freq  -6829 +/-   3 delay    12 +/-   0
	ptp4l[14831.282]: rms   15 max   28 freq  -6811 +/-   7 delay    12 +/-   0
	ptp4l[14832.283]: rms    4 max    8 freq  -6795 +/-   5 delay    12 +/-   0
	ptp4l[14833.284]: rms    7 max   10 freq  -6783 +/-   3 delay    12 +/-   0
	ptp4l[14834.285]: rms    2 max    5 freq  -6788 +/-   3 delay    13 +/-   0
	ptp4l[14835.286]: rms    3 max    5 freq  -6793 +/-   3 delay    13 +/-   0
	```

	Please make it sure that the secondary sytem refers to the primary's precision time
	by checking if there is the __"new foreign master"__ message in a log<br>
	Note that the rms value should be decreasing with each line and should go 
	down to single digits.

5. While step #4 is still executing, synchronize wall clocks of the system as the root user.<br>
	On both the primary & secondary systems, run the following command:
	```phc2sys -s enp176s0 -O 0 -R 8 -u 8 -m```<br>
	In the above command, replace enp176s0 with the ethernet interface that
	you find on your systems where the network cable is connected to.
	Steps 4 & 5 will synchronize the wall clocks of the two systems. You can
	stop these commands from executing by pressing Ctrl+C on both systems. Now
	you are ready to execute the vsync_test.

Installing and running the programs:<br>
1. Copy the release folder contents to both primary and secondary systems.<br>
2. On both systems, go to the directory where these files exist and set environment variable:
export LD_LIBRARY_PATH=.<br>
3. On the primary system, run it as follows:
	```./vsync_test pri [PTP_ETH_Interface]```<br>
4. On the secondary system, run it as follows:
	```./vsync_test sec PTP_ETH_Interface [Primary_ETH_MAC_Addr] [sync after # us]```

This program runs in server mode on the primary system and in client mode on the 
secondary system.

If using PTP protocol to communicate between primary and secondary, there are some extra
parameters required. The primary system must identify the PTP Interface (ex: enp176s0). The
secondary system also requires its PTP interface as well as this port's ethernet MAC address.
An example of PTP communication between primary and secondary looks like this:
On the primary system, run it as follows:
	```./vsync_test pri enp176s0```<br>
On the secondary system, run it as follows:
	```./vsync_test sec enp176s0 84:47:09:04:eb:0e```

In the above examples, we were just sync'ing the secondary system once with the primary.
However, due to reference clock differences, we see that there is a drift pretty much as
soon as we have sync'd them. We also have another capability which allows us to resync 
as soon as it goes above a threshold. For example:  
On the primary system, run it as follows:
	```./vsync_test pri enp176s0```<br>
On the secondary system, run it as follows:
	```./vsync_test sec enp176s0 84:47:09:04:eb:0e 100```

In the above example, secondary system would sync with the primary once but after that
it would constantly check to see if the drift is going above 100 us. As soon as it does,
the secondary system would automatically resync with the primary. It is recommended to
select a large number like 60 or 100 since if we use a smaller number, then we will be
resyncing all the time. On the other hand, if we chose a really big number like 1000 us,
then we are allowing the systems to get out of sync from each other for 1 ms. On a
Tiger Lake system, 100 us difference is usually happening in around 60-70 seconds or
about a minute. So this program would automatically resync with the primary every minute
or so.
