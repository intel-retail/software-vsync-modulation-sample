This program allows two systems to synchronize their vertical sync firing times.

Generating Doxygen documents:<br>
Please install doxygen and graphviz packages before generating Doxygen documents:
	```sudo apt install doxygen graphviz```
1. Type ```make doxygen``` from the main directory. It will genarte SW Genlock doxygen documents
	to output/doxygen folder.
2. Open output/doxygen/html/index.html with a web-browser

Building steps:<br>
1. Type ```make release``` from the main directory. It compiles everything and creates a 
   output/release folder which can then be copied to the target systems.

Building of this program has been successfully tested on both Ubuntu 2x and Fedora 30.<br>
Please note that this document was written based on Ubuntu

Preparing two systems for PTP communication:<br>
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
