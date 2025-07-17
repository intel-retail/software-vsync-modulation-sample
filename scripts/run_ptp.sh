#!/bin/bash
# *
# * Copyright © 2024 Intel Corporation
# *
# * Permission is hereby granted, free of charge, to any person obtaining a
# * copy of this software and associated documentation files (the "Software"),
# * to deal in the Software without restriction, including without limitation
# * the rights to use, copy, modify, merge, publish, distribute, sublicense,
# * and/or sell copies of the Software, and to permit persons to whom the
# * Software is furnished to do so, subject to the following conditions:
# *
# * The above copyright notice and this permission notice (including the next
# * paragraph) shall be included in all copies or substantial portions of the
# * Software.
# *
# * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# * IN THE SOFTWARE.
#
# This script runs ptp4l and phc2sys on both the primary and secondary machines. It should be executed
# from the primary machine.
#
# Script requires command requires passwordless sudo access on primary and secondary machines.  Follow steps to get that
#   $ sudo visudo
#   Add below line towards the end after @includedir /etc/sudoers.d
#   user_id ALL=(ALL) NOPASSWD: ALL
#
# The primary machine must have passwordless SSH login enabled for the secondary to allow remote
# command execution.
#
# To set this up, follow these steps on the primary machine:
#             (primary)$ ssh-keygen
#             (primary)$ ssh-copy-id user@secondary
#
# Make sure to update/change configuration variables for both primary and secondary machines as necessary.
#
# After executing the script, the ptp4l and phc2sys will start synchrnonizing time on both machines.
# The script will wait for the user to press any key before terminating all four processes on both
# primary and secondary machines.

# TIPS:
# Use this command to check which network card support ptp
#    $ ls /sys/class/net/*/device/ptp*
#
# Manually setting system and PTP time to different values. These commands are for testing purposes only.
# Set the system date and time:
#    $ sudo date --set="yyyy-mm-dd hh:mm:ss"
# Set the PTP time (number of seconds since 1970):
#    $ sudo phc_ctl /dev/ptp0 set 10000000000

# Determine the directory of the currently executing script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"


# Include the variables from config.sh
source "$SCRIPT_DIR/config.sh"

# Update time
echo "Updating time on primary with ntp.ubuntu.com ..."
$SUDO ntpdate ntp.ubuntu.com

echo "Disabling ntp on primary"
$SUDO timedatectl set-ntp no

echo "Disabling ntp on secondary"
ssh $SECONDARY_ADDRESS "$SUDO timedatectl set-ntp no"

# timedatectl set-ntp no

# Ensure the directory on secondary exists
ssh $SECONDARY_ADDRESS "mkdir -p $CLIENT_DIR"

# Copy config file to secondary system
scp "$PRIMARY_PROJ_PATH/resources/gPTP.cfg" "$SECONDARY_ADDRESS:$CLIENT_DIR/" 2>/dev/null

echo "PRIMARY Commands:"
COMMAND="ptp4l -i $PRIMARY_INERFACE -m -f $PRIMARY_PROJ_PATH/resources/gPTP.cfg"
echo "      ptp4l : $COMMAND"
# Commands to run on the primary machine
PRIMARY_COMMANDS_PTP="$SUDO $COMMAND 2>&1 | tee $LOG_DIR$PRIMARY_LOG_PTP & "

COMMAND="phc2sys -c $PRIMARY_INERFACE -s CLOCK_REALTIME -w -O 0 -m -f $PRIMARY_PROJ_PATH/resources/gPTP.cfg"
echo "    phc2sys : $COMMAND"
# Update primary ptp clock with system clock time
PRIMARY_COMMANDS_PHC="$SUDO $COMMAND 2>&1 | tee $LOG_DIR$PRIMARY_LOG_PHC &"

echo "SECONDARY Commands:"
COMMAND="ptp4l -i $SECONDARY_INTERFACE -s -m -f $CLIENT_DIR/gPTP.cfg"
echo "      ptp4l : $COMMAND"
# Commands to run on the secondary machine
SECONDARY_COMMANDS_PTP="$SUDO $COMMAND &"

COMMAND="phc2sys -s $SECONDARY_INTERFACE -c CLOCK_REALTIME -O 0 -m -f $CLIENT_DIR/gPTP.cfg"
echo "    phc2sys : $COMMAND"
echo ""
# Update secondary system clock time with ptp time
SECONDARY_COMMANDS_PHC="$SUDO $COMMAND &"

# First sync Primary PTP clock with it's system clock
eval $PRIMARY_COMMANDS_PTP
# Give some time for ptp4l to start
sleep 4

eval $PRIMARY_COMMANDS_PHC
echo "Waiting on Primary to sync PTP with real time clock"
# 10 second are enough.  Adjust accordingly
sleep 10

# Wakeup remote screen (Uncomment if want to wake up PC via ssh at regular interval of 5 sec)
#watch -n 5 ssh $SECONDARY_ADDRESS dbus-send --type=method_call --dest=org.gnome.ScreenSaver /org/gnome/ScreenSaver org.gnome.ScreenSaver.SetActive boolean:false &

# Run commands on the secondary machine via SSH
echo "Running commands on the secondary machine..."
ssh $SECONDARY_ADDRESS "$SECONDARY_COMMANDS_PTP" 2>&1 | tee  $LOG_DIR$SECONDARY_LOG_PTP  &
# Give some time for ptp4l on secondary to sysncup with primary
sleep 10

# Now start syncing real time clock on secondary with it's PTP clock
ssh $SECONDARY_ADDRESS "$SECONDARY_COMMANDS_PHC" 2>&1 | tee  $LOG_DIR$SECONDARY_LOG_PHC  &

# Define cleanup function
cleanup() {
    echo "Cleaning up..."
    source "$SCRIPT_DIR/stop_ptp.sh"
    echo "Cleanup complete."
    exit 0
}

# Trap SIGINT (Ctrl+C) and call cleanup function
trap cleanup SIGINT

echo "Running... Press Ctrl+C to terminate."
while true; do
    sleep 1
done

