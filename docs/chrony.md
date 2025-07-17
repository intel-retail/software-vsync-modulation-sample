
# Synchronization Real-time clock Using Chrony (For Non-PTP Interfaces)

For systems with Ethernet adapters that do not support PTP (Precision Time Protocol), **Chrony** offers a practical alternative for clock synchronization. This method ensures consistent time alignment across systems required by the software genlock feature.

However, it is important to note that **Chrony-based synchronization may not achieve the same level of accuracy as hardware-assisted PTP**, as it relies entirely on software. Its precision can be affected by factors such as **network latency, interface speed, and overall system load**. For applications requiring tight synchronization margins, PTP remains the preferred option when supported.

#### Primary System (Time Server)

1. **Install Chrony**:
   ```bash
   sudo apt install chrony
   ```
2. **Edit the configuration file /etc/chrony/chrony.conf**:
   ```
   # Allow clients from the subnet 192.168.1.0
   allow 192.168.1.0/24  #

   # (Optional) Bind to a specific network interface
   binddevice <ethernet_interface>
   hwtimestamp <ethernet_interface>
   local stratum 8

   # Make sure primary Chrony doesn't sync with any external pool/server.  Disable/remove all of them
   # pool pool.ntp.org iburst
   # server corp.intel.com iburst
   ```

3. **Restart the Chrony service**:
   ```bash
   sudo systemctl restart chrony
   ```

4. **(If a firewall is active) Allow NTP traffic**:
   ```bash
   sudo ufw allow 123/udp
   ```

####  Secondary System (Time Client)

1. **Install Chrony**:
   ```bash
   sudo apt install chrony
   ```
2. **Edit the configuration file /etc/chrony/chrony.conf**:
   Replace any existing pool or server entries with:
   ```
   server <primary_system_IP> iburst minpoll 1 maxpoll 1
   binddevice <ethernet_interface>
   hwtimestamp <ethernet_interface>
   ```

4. **Restart the Chrony service**:
   ```bash
   sudo systemctl restart chrony
   ```

5. **(If a firewall is active) Allow NTP traffic**:
   ```bash
   sudo ufw allow 123/udp

#### Verifying Synchronization (on the Client)
Use the following commands to verify synchronization status:

##### On Primary:
```bash
$ chronyc tracking
Reference ID    : 7F7F0101 ()
Stratum         : 8

$ chronyc sources
MS Name/IP address         Stratum Poll Reach LastRx Last sample
===============================================================================

```
The reference ID in the () should be blank and sources list should be empty.

##### On Secondary:

The output should list the primary system as the time source (Assuming secondary system address is 192.168.1.100):
```bash
$ chronyc tracking
Reference ID    : C0A80A02 (192.168.1.4)
Stratum         : 9

$ chronyc sources
MS Name/IP address         Stratum Poll Reach LastRx Last sample
===============================================================================
^* 192.168.1.4     8   9   377   354    +7943ns[+7967ns] +/-   45us
```
The reference ID in the () should point to primary IP address and sources list should only have primary IP entry.  The ^* symbol indicates that synchronization is active and successful.

##### Set Real-Time Scheduling Priority

Chrony should be run with real-time scheduling policy (SCHED_FIFO) and high priority.  On both primary and secondary run

```bash
$ sudo systemctl edit chrony
```

Add the following:

```
### Editing /etc/systemd/system/chrony.service.d/override.conf
### Anything between here and the comment below will become the new contents of the file

[Service]
Nice=-20
CPUSchedulingPolicy=FIFO

### Lines below this comment will be discarded
```

Then reload and restart:

```
$ sudo systemctl daemon-reexec
$ sudo systemctl restart chrony
```
