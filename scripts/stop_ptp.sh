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


# Determine the directory of the currently executing script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Include the variables from config.sh
source "$SCRIPT_DIR/config.sh"

# Commands to stop ptp4l and phc2sys on the primary machine
PRIMARY_STOP_COMMANDS="
    $SUDO pkill -SIGINT  ptp4l 2>&1 | tee  -a $LOG_DIR$PRIMARY_LOG_PTP &
    $SUDO pkill -SIGINT phc2sys 2>&1 | tee -a $LOG_DIR$PRIMARY_LOG_PHC
"

# Commands to stop ptp4l and phc2sys on the secondary machine
SECONDARY_STOP_COMMANDS_PTP="$SUDO pkill -SIGINT  ptp4l"
SECONDARY_STOP_COMMANDS_PHC="$SUDO pkill -SIGINT  phc2sys"

# Stop commands on the primary machine
echo "Stopping commands on the primary machine..."
eval $PRIMARY_STOP_COMMANDS

# Stop commands on the secondary machine via SSH
echo "Stopping commands on the secondary machine..."
ssh $SECONDARY_ADDRESS $SECONDARY_STOP_COMMANDS_PTP 2>&1 | tee  -a $LOG_DIR$SECONDARY_LOG_PTP
ssh $SECONDARY_ADDRESS $SECONDARY_STOP_COMMANDS_PHC  2>&1 | tee  -a $LOG_DIR$SECONDARY_LOG_PHC

echo "All processes terminated."

