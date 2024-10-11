 Copyright (C) 2023 Intel Corporation
 SPDX-License-Identifier: MIT

# Introduction SW Genlock

SW Genlock is a software solution designed to synchronize the display outputs of multiple computer systems to single digit microsecond precision, enabling seamless visual outputs across an array of screens. This technology is particularly beneficial in settings that require highly synchronized video outputs, such as video walls, virtual reality setups, digital signage, and other multi-display configurations.

## Key Features of SW Genlock

- **High Precision Synchronization**: Ensures that vertical sync (vblank) intervals occur simultaneously across all connected systems, maintaining the integrity of visual displays.
- **Flexible Integration**: Offers both dynamic (.so) and static (.a) linking options to accommodate various application needs.
- **Comprehensive Tools**: Includes a suite of tools and libraries that facilitate the synchronization process, such as obtaining vblank timestamps and adjusting the phase-locked loop (PLL) frequencies.
- **Extensive Compatibility**: Supports systems equipped with Precision Time Protocol (PTP), allowing for precise clock synchronization over network infrastructures.
- **User-Centric Utilities**: Provides reference applications that demonstrate effective usage of the library, assisting users in integrating Genlock capabilities into their own projects.

The SW Genlock system operates by aligning the internal clocks of all systems involved, ensuring that every display refresh is triggered simultaneously. This alignment is achieved through the utilization of industry-standard protocols and our advanced proprietary algorithms, which manage the synchronization over ethernet or direct network connections.

## Known Issues

Users of this SW Genlock libraries may need to adjust the value for SHIFT. In testing, on various displays a valaue
that is too large can cause flickering on screen when synchronized. To resolve this, adjust the value of SHIFT. The adjustment can be made by:

1) Editing the default value in [lib/common.h](./lib/common.h)
2) Providing a new value for SHIFT when running the sample application:

```bash
synctest -s <shift_value>
```

# Installation

Installing SW Genlock involves a few straightforward steps. This section guides through the process of setting up SW Genlock on target systems, ensuring that user have everything needed to start synchronizing displays.

## System Requirements

Before the user begin the installation, ensure the systems meet the following criteria:

The build system assumes the following, prior to executing the build steps:

1) The system has an [Ubuntu OS](https://ubuntu.com/tutorials/install-ubuntu-desktop#1-overview)
1) Libraries installed: `sudo apt install -y git build-essential make`
1) [Disable secure boot](https://wiki.ubuntu.com/UEFI/SecureBoot/DKMS)
1) MUST apply the [PLL kernel patch](./resources/0001-dpll-Provide-a-command-line-param-to-config-if-PLLs-.patch).
   Compile the kernel and update either the entire kernel or just the i915.ko module, depending on the Linux configuration. Note: In some setups, the i915.ko module is integrated into the
   initrd.img, and updating it in /lib/module/... might not reflect the changes.
   - Following the application of the patch, edit the grub settings:
 ```console
 sudo vi /etc/grub/default
 # add i915.share_dplls=0 to the GRUB_CMDLINE_LINUX options:
 # GRUB_CMDLINE_LINUX="i915.share_dplls=0"
 # Save and exit, then update grub
 update-grub
 ```

1) Apply **[the monotonic timstamp patch](./resources/0001-Revert-drm-vblank-remove-drm_timestamp_monotonic-par.patch)**
 to Linux kernel i915 driver which allows it to provide vsync timestamps in real time
 instead of the default monotonic time.<br>
 Note that the monotonic timestamp patch is generated based on Linux v6.4 and has been tested on Linux v6.4 and v6.5.<br>
 Please follow the steps to disable monotonic timestamp after installing the local built Linux image on a target. Compile the kernel and update either the entire kernel or just the   drm.ko module based on the installed Linux configuration. <br>
  1. Add ```drm.timestamp_monotonic=0``` option to **GRUB_CMDLINE_LINUX** in /etc/default/grub
  2. Update **GRUB_DEFAULT** in /etc/default/grub to address the local built Linux image
  3. Run ```update-grub``` to apply the change
  4. Reboot a target
1) Turn off NTP time synchronization service by using this command:
 ```timedatectl set-ntp no```
1) All the involved systems must support PTP time synchronization via ethernet (e.g ptp4l, phc2sys)
   To verify if an ethernet interface supports PTP, the `ethtool` linux tool can be used. Run the following command, replacing `eth0` with the relevant interface name:

   ```console
   ethtool -T eth0
   ```

   If the interface supports PTP, the output will display specific PTP hardware clock information.
   To ensure proper synchronization, the Ethernet interfaces on the involved systems should be directly connected either via a crossover cable or through a PTP-supported router or switch. If the systems only have a single network interface, it may be necessary to add a separate network adapter, such as a USB-to-Ethernet or USB-to-WiFi dongle, to maintain SSH access and connectivity to external networks.
1) Displays must be at same refresh rate (e.g 60Hz)

## Build steps

1) Git clone this repository
1) `sudo apt install libdrm-dev libpciaccess-dev`
    1) If the directory /usr/include/drm does not exist, it may be necessary to run the command`sudo apt install -y linux-libc-dev`
2) Type `make` from the main directory. It compiles everything and creates library and executable binaries in respective folders along with code which can then be copied to the target systems.

# Software Components

## Vsync Library

The Vsync library is distributed in both dynamic (.so) and static (.a) binary formats, allowing users to choose between dynamic and static linking depending on their application requirements. This library includes essential functions such as retrieving vblank timestamps and adjusting the vblank timestamp drift by specified microseconds. Additionally, all operations related to Phase-Locked Loop (PLL) programming are encapsulated within the library, providing a comprehensive suite of tools for managing display synchronization.

## Reference Applications

Accompanying the library, three reference applications are provided to demonstrate how to utilize the library effectively. These include:

- A `vsync test` app that runs in either primary mode or secondary mode.  primary mode is run on a single PC whereas all other PCs are run as secondary mode with parameters pointing to primary mode system ethernet address.  The communication is done either in ethernet mode or IP address mode.

```console
Usage: ./vsync_test [-m mode] [-i interface] [-c mac_address] [-d delta] [-p pipe] [-s shift] [-v loglevel] [-h]
Options:
  -m mode        Mode of operation: pri, sec (default: pri)
  -i interface   Network interface to listen on (primary) or connect to (secondary) (default: 127.0.0.1)
  -c mac_address MAC address of the network interface to connect to. Applicable to ethernet interface mode only.
  -d delta       Drift time in microseconds to allow before pll reprogramming (default: 100 us)
  -p pipe        Pipe to get stamps for.  4 (all) or 0,1,2 ... (default: 0)
  -s shift       PLL frequency change fraction (default: 0.1)
  -v loglevel    Log level: error, warning, info, debug or trace (default: info)
  -h             Display this help message
```

* A `vbltest` app to print average vblank period between sync.  This is useful in verification and validation.

```console
[INFO] Vbltest Version: 2.0.0
 Usage: ./vbltest [-p pipe] [-c vsync_count] [-v loglevel] [-h]
 Options:
  -p pipe        Pipe to get stamps for.  0,1,2 ... (default: 0)
  -c vsync_count Number of vsyncs to get timestamp for (default: 300)
  -v loglevel    Log level: error, warning, info, debug or trace (default: info)
  -h             Display this help message
```

* A `synctest` app to drift vblank clock on a single display by certain period such as 1000 microseconds (or 1.0 ms).

```console
[INFO] Synctest Version: 2.0.0
Usage: ./synctest [-p pipe] [-d delta] [-s shift] [-v loglevel] [-h]
Options:
-p pipe        Pipe to get stamps for.  0,1,2 ... (default: 0)
-d delta       Drift time in us to achieve (default: 1000 us) e.g 1000 us = 1.0 ms
-s shift       PLL frequency change fraction (default: 0.1)
-v loglevel    Log level: error, warning, info, debug or trace (default: info)
-h             Display this help message
```

**synctest sample output:**

```console
./synctest
[INFO] Synctest Version: 2.0.0
[INFO] DRM Info:
[INFO]   CRTCs found: 4
[INFO]    Pipe:  0, CRTC ID:   80, Mode Valid: Yes, Mode Name: , Position: (   0,    0), Resolution: 1920x1080, Refresh Rate: 60.00 Hz
[INFO]    Pipe:  1, CRTC ID:  131, Mode Valid:  No, Mode Name: , Position: (   0,    0), Resolution:    0x0   , Refresh Rate: 0.00 Hz
[INFO]    Pipe:  2, CRTC ID:  182, Mode Valid:  No, Mode Name: , Position: (   0,    0), Resolution:    0x0   , Refresh Rate: 0.00 Hz
[INFO]    Pipe:  3, CRTC ID:  233, Mode Valid:  No, Mode Name: , Position: (   0,    0), Resolution:    0x0   , Refresh Rate: 0.00 Hz
[INFO]   Connectors found: 6
[INFO]    Connector: 0    (ID: 236 ), Type: 11   (HDMI-A      ), Type ID: 1   , Connection: Disconnected
[INFO]    Connector: 1    (ID: 246 ), Type: 11   (HDMI-A      ), Type ID: 2   , Connection: Connected
[INFO]    Encoder ID: 245, CRTC ID: 80
[INFO]    Connector: 2    (ID: 250 ), Type: 10   (DisplayPort ), Type ID: 1   , Connection: Disconnected
[INFO]    Connector: 3    (ID: 259 ), Type: 11   (HDMI-A      ), Type ID: 3   , Connection: Disconnected
[INFO]    Connector: 4    (ID: 263 ), Type: 10   (DisplayPort ), Type ID: 2   , Connection: Disconnected
[INFO]    Connector: 5    (ID: 272 ), Type: 10   (DisplayPort ), Type ID: 3   , Connection: Disconnected
[INFO] VBlank interval before starting synchronization: 16.667000 ms
[INFO] VBlank interval during synchronization ->
[INFO]   VBlank interval on pipe 0 is 16.6650 ms
[INFO]   VBlank interval on pipe 0 is 16.6650 ms
[INFO]   VBlank interval on pipe 0 is 16.6650 ms
[INFO]   VBlank interval on pipe 0 is 16.6650 ms
[INFO]   VBlank interval on pipe 0 is 16.6640 ms
[INFO]   VBlank interval on pipe 0 is 16.6650 ms
[INFO]   VBlank interval on pipe 0 is 16.6650 ms
[INFO]   VBlank interval on pipe 0 is 16.6660 ms
[INFO] VBlank interval after synchronization ends: 16.667000 ms
```

# Generating Doxygen documents

Please install doxygen and graphviz packages before generating Doxygen documents:
 ```sudo apt install doxygen graphviz```

1. Type ```make doxygen VERSION="1.2.3"``` from the main directory. It will generate SW Genlock doxygen documents
 to output/doxygen folder. Change the version to be that of the release number for the project.
2. Open output/doxygen/html/index.html with a web-browser

# Running on single system

The reference applications vbltest and synctest are designed to operate on a single system. The vbltest app displays the average vblank period for a specific pipe, while synctest allows user to introduce a specific drift in the vblank timing for a pipe. Synctest serves as a practical tool to verify the effectiveness of the synchronization methodology.

1) On the system, go to the directory where these files exist and set environment variable:
2) For dynamic linking with the .so library, it's necessary to set the LD_LIBRARY_PATH environment variable so that applications can locate the vsync dynamic library. This step is not required when using static linking.

```console
export LD_LIBRARY_PATH=`pwd`/lib
``````

3) On the primary system, run accompanied reference apps as follows:
4) e.g synctest
 ```console
   ./synctest
   ```

Building of this program has been successfully tested on both Ubuntu 2x and Fedora 30.<br>

# Synchronization between two systems

Synchronizing displays across two systems involves a two-step process. Firstly, the Real Time Clocks of both systems need to be kept in synchronized state using the ptp4l Linux tool. Subsequently, the vsync test app should be run in primary mode on one system and in secondary mode on the other, ensuring that the vblank of the secondary system remains synchronized with that of the primary system.

## Synchronizing Clocks between two systems

The SW Genlock feature requires that all participating systems' clocks be synchronized to the nanosecond level. Kernel patches included with the software return vblank timestamps based on the system clock rather than the kernel's monotonic clock, which tracks time from when the kernel is loaded. Accurate synchronization is crucial because SW Genlock shares vblank timestamps from the primary system with secondary systems. This synchronization is achieved using the Linux ptp4l tool.  There should be a network cable directly connecting the ethernet ports of the two
systems.

### Clock Configuration in PTP-Supported Systems

Systems supporting PTP typically have two clocks: the system's real-time clock and another clock on the Ethernet card. The synchronization process involves:

1. **Primary System Real-Time Clock Synchronization**:
   - Initially synchronize the primary system's real-time clock with an NTP server.
   - Then sync the Ethernet PTP clock with the primary system's real-time clock.

2. **Secondary Systems Synchronization**:
   - Sync the secondary systems' Ethernet PTP clocks with the primary's Ethernet PTP clock.
   - Sync their system real-time clocks with their Ethernet PTP clocks, which are already synchronized with the primary system.

This setup ensures all secondary system's real-time clocks are synchronized with the primary system's real-time clock. Synchronization within a system (system real-time clock to Ethernet PTP clock) is managed using the `phc2sys` tool, while inter-system synchronization (across Ethernet interfaces) utilizes the `ptp4l` tool.

<figure><img src="./resources/images/ptp4l.png" alt="PTP4L Setup" /><figcaption>PTP4L Setup.</figcaption></figure>

It's also important to turn off NTP time synchronization service by using this command:
 ```$ timedatectl set-ntp no```

### Command Sets for PTP time Synchronization

Assuming the primary Ethernet interface is named `enp87s0` and the secondary interface is named `enp89s0`, the following commands are to be executed under root or with sudo:

**Primary System Commands:**

- **ptp4l:**

```shell
ptp4l -i enp87s0 -m -f ./resources/gPTP.cfg
```

- **phc2sys:**

```shell
phc2sys -c enp87s0 -s CLOCK_REALTIME -w -O 0 -m -f ./resources/gPTP.cfg
```

**Secondary System Commands:**

- **ptp4l:**

```shell
ptp4l -i enp89s0 -s -m -f ./resources/gPTP.cfg
```

- **phc2sys:**

```shell
phc2sys -s enp89s0 -c CLOCK_REALTIME -O 0 -m -f ./resources/gPTP.cfg
```

The output of ptp4l on the primary system after running the above command should look
 like this:<br>

```console
ptp4l[13416.983]: selected /dev/ptp1 as PTP clock
ptp4l[13417.002]: port 1: INITIALIZING to LISTENING on INIT_COMPLETE
ptp4l[13417.003]: port 0: INITIALIZING to LISTENING on INIT_COMPLETE
ptp4l[13420.445]: port 1: LISTENING to MASTER on ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES
ptp4l[13420.445]: selected local clock 844709.fffe.04f07f as best master
ptp4l[13420.445]: assuming the grand master role
```

The output of ptp4l on the secondary system after running the above command should
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
 by checking if there is the **"new foreign master"** message in a log<br>
 Note that the rms value should be decreasing with each line and should go
 down to single digits.

For user convenience, an automation Bash script is available in the scripts folder to facilitate ptp4l synchronization on both primary and secondary systems. Users simply need to update the `config.sh` file with relevant details such as the secondary system's address, username, and Ethernet address, among others. After these updates, run the scripts/run_ptp.sh script from the primary system. This script will automatically execute the ptp4l and phc2sys tools on both the primary and secondary systems in their respective modes. Note that the script requires the secondary system to enable passwordless SSH to execute ptp4l commands through the SSH interface.

Since the ptp4l and phc2sys tools require root privileges or sudo access, users need to provide their sudo password in the config.sh file. As an alternative, setting up passwordless sudo can streamline this process. Here's how to enable passwordless sudo:

Open a terminal and type `sudo visudo` to edit the sudoers file.
Add the following line to the file at the end, replacing username with actual username:

```bash
username ALL=(ALL) NOPASSWD: ALL
```

Save and close the file to apply the changes.
This adjustment allows the automation scripts to run without requiring a sudo password each time, simplifying the setup for ptp4l and phc2sys synchronizations.

## Synchronizing Display VBlanks

Once the system clocks are synchronized, user can proceed to run the vsync test tool on both the primary and secondary systems.
Installing and running the programs:<br>

1. Copy the release folder contents to both primary and secondary systems.<br>
2. On both systems, go to the directory where these files exist and set environment variable:
export LD_LIBRARY_PATH=.<br>
3. On the primary system, run it as follows:
 ```console
 ./vsync_test -m pri -i [PTP_ETH_Interface]
  ```

5. On the secondary system, run it as follows:
 ```console
 ./vsync_test -m sec -i PTP_ETH_Interface -c [Primary_ETH_MAC_Addr] -d [sync after # us of drift]
  ```

This program runs in server mode on the primary system and in client mode on the
secondary system.

<figure><img src="./resources/images/vsync_test.png" alt="VBlank synchronization setup" /><figcaption>VBlank synchronization setup.</figcaption></figure>

If using PTP protocol to communicate between primary and secondary, there are some extra
parameters required. The primary system must identify the PTP Interface (ex: enp176s0). The
secondary system also requires its PTP interface as well as this port's ethernet MAC address.
An example of PTP communication between primary and secondary looks like this:
On the primary system, run it as follows:
 ```console
   ./vsync_test -m pri -i enp176s0
    ```
On the secondary system, run it as follows:
 ```console
  ./vsync_test -m sec -i enp176s0 -c 84:47:09:04:eb:0e
   ```

In the above examples, we were just sync'ing the secondary system once with the primary.
However, due to reference clock differences, we see that there is a drift pretty much as
soon as we have sync'd them. We also have another capability which allows us to resync
as soon as it goes above a threshold. For example:
On the primary system, run it as follows:
 ```./vsync_test -m pri -i enp176s0```<br>
On the secondary system, run it as follows:
 ```./vsync_test -m sec -i enp176s0 -c 84:47:09:04:eb:0e 100```

In the above example, secondary system would sync with the primary once but after that
it would constantly check to see if the drift is going above 100 us. As soon as it does,
the secondary system would automatically resync with the primary. It is recommended to
select a large number like 60 or 100 since if we use a smaller number, then we will be
resyncing all the time. On the other hand, if we chose a really big number like 1000 us,
then we are allowing the systems to get out of sync from each other for 1 ms. On a
Tiger Lake system, 100 us difference is usually happening in around 60-70 seconds or
about a minute. So this program would automatically resync with the primary every minute
or so.

Similar to ptp4l, an automation script is provided to setup vsync primary and secondary on both systems.  see ./scripts/run_vsync.sh.  Make sure to update config.sh accordingly unless if it's already updated for run_ptp.sh.

Similar to ptp4l, For user convenience, an automation Bash script is available in the scripts folder to facilitate vsync synchronization on both primary and secondary systems. Users simply need to update the `config.sh` file with relevant details such as the secondary system's address, username, and Ethernet address, among others. After these updates, run the scripts/run_vsync.sh script from the primary system. This script will automatically copy vblank_sync to secondary under ~/.vblanksync directory and execute the vblank_sync tool on both the primary and secondary systems in their respective modes. Note that the script requires the secondary system to enable passwordless SSH to execute ptp4l commands through the SSH interface.

```console
./scripts/run_vsync.sh
```

The console will display logs from both systems, but each process, including ptp4l and phc2sys, will also generate its own separate log file. These files can be found in the ./logs directory at the root level.

## Running vsync_test as a Regular User without sudo

While the ptp synchronization section provides methods for configuring passwordless sudo, it may be preferable to operate without using sudo or root privileges. The `vsync_test` application, which requires access to protected system resources for memory-mapped I/O (MMIO) operations at `/sys/bus/pci/devices/0000:00:02.0/resource0`, can be configured to run under regular user permissions. This can be achieved by modifying system permissions either through direct file permission changes or by adjusting user group memberships. Note that the resource path might change in future versions of the kernel. The resource path can be verified by running the application with the `strace` tool in Linux as a regular user and checking for permission denied errors or by examining `/proc/processid/maps` for resource mappings.

### Option 1: Modifying File Permissions

Altering the file permissions to allow all users to read and write to the device resource removes the requirement for root access. This method should be used with caution as it can lower the security level of the system, especially in environments with multiple users or in production settings.

To change the permissions, the following command can be executed:

```bash
sudo chmod o+rw /sys/bus/pci/devices/0000:00:02.0/resource0
```

This command adjusts the permissions to permit all users (o) read and write (rw) access to the specified resource.

### Option 2: Configuring User Group Permissions and Creating a Persistent udev Rule

A more secure method is to assign the resource to a specific user group, such as the `video` group, and add the user to that group. This method confines permissions to a controlled group of users. To ensure the changes persist after a reboot, a `udev` rule can be created.

#### Step 1: Add the Resource to the Video Group

Change the group ownership of the resource file to the `video` group using the following command:

```bash
sudo chgrp video /sys/bus/pci/devices/0000:00:02.0/resource0
```

#### Step 2: Modify Group Permissions

Set the group permissions to allow read and write access with:

```bash
sudo chmod g+rw /sys/bus/pci/devices/0000:00:02.0/resource0
```

#### Step 3: Add the User to the Video Group

Add the user to the video group using this command (replace username with the actual user name):

```bash
sudo usermod -a -G video username
```

Log out and log back in, or start a new session, for the group changes to take effect.

#### Step 4: Create a udev Rule for Persistent Permissions

To make the permissions persist after reboot, create a `udev` rule by adding a new rule file in `/etc/udev/rules.d/`:

```bash
echo 'ACTION=="add", SUBSYSTEM=="pci", KERNEL=="0000:00:02.0", RUN+="/bin/chgrp video %S%p/resource0", RUN+="/bin/chmod 0660 %S%p/resource0"' | sudo tee /etc/udev/rules.d/99-vsync-access.rules
```

This rule ensures that the permissions are correctly set when the system boots.

#### Step 5: Reload udev Rules

After creating the rule, reload the udev rules to apply them immediately without rebooting:

```bash
sudo udevadm control --reload-rules && sudo udevadm trigger -s pci --action=add
```

#### Verifying the Changes

After applying one of the above methods, confirm that the user has the necessary permissions by running the vsync_test application without elevated privileges. If the setup is correct, the application should run without requiring sudo or root access.

## Running ptp4l as a Regular User without sudo

Similarly, the ptp4l command can be executed by a regular user. This can be done by either altering the permissions or modifying the group for `/dev/ptp0`, or by creating a udev rule to ensure persistent permissions.

```bash
echo 'SUBSYSTEM=="ptp", KERNEL=="ptp0", GROUP="video", MODE="0660"' | sudo tee /etc/udev/rules.d/99-ptp-access.rules
```

Reload udev Rules

```bash
sudo udevadm control --reload-rules &&  sudo udevadm trigger
```

However, `phc2sys` will still need to be run with root or sudo privileges as it involves modifying system time. Running it under a normal user account could be possible if permissions to update the system clock are granted to the application.

# Debug Tools

When performing debugging, it is helpful to have the following libraries installed:

```bash
apt install -y intel-gpu-tools edid-decode
```

## Debug Tool Usage

When encountering unexpected behavior that requires additional debugging, the installed tools can assist in narrowing down the potential problem.

**[intel-gpu-tools](https://cgit.freedesktop.org/xorg/app/intel-gpu-tools/)**
Provides tools to read and write to registers, it must be run as the root user. Example:

```console
intel_reg dump --all > /tmp/reg_dump.txt

# output excerpt
                    GEN6_RP_CONTROL (0x0000a024): 0x00000400
Gen6 disabled
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

To operate edid-decode, it is typically necessary to supply a binary EDID file or data. Here's an example of how to use it:

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
