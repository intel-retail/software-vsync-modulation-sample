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
# The script runs vsync_test app on primary and secondary machines. The script is supposed
# to be run from primary PC.
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
# After executing the script, the vsync_test will be run on both machines; pri & sec modes.
# The script will wait for the user to press any key before terminating both processes

# Determine the directory of the currently executing script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Include the variables from config.sh
source "$SCRIPT_DIR/config.sh"

# Ensure the client directory exists
ssh $SECONDARY_ADDRESS "mkdir -p $CLIENT_DIR"

# Copy executable to the client
echo "Copying executables to the Secondary ..."
scp "$PRIMARY_PROJ_PATH/$VSYNCTEST_APP" "$PRIMARY_PROJ_PATH/$SYNCTEST_APP" "$PRIMARY_PROJ_PATH/$VBLTEST_APP" "$SECONDARY_ADDRESS:$CLIENT_DIR/" 2>/dev/null

COMMAND="$PRIMARY_PROJ_PATH/$VSYNCTEST_APP -m pri -i $PRIMARY_INERFACE -e $PRIMARY_DEVICE "
echo "PRIMARY Command  : $COMMAND"
PRIMARY_COMMANDS_VSYNC="$SUDO $COMMAND 2>&1 | tee  $LOG_DIR$PRIMARY_LOG_VSYNC &"

COMMAND="$CLIENT_DIR/$VSYNCTEST -m sec -i $SECONDARY_INTERFACE -c $PRIMARY_ETH_ADDR -e $SECONDARY_DEVICE -d 100 -s 0.01"
echo "SECONDARY Command: $COMMAND"
SECONDARY_COMMANDS_VSYNC="$SUDO $COMMAND &"

# Run commands on the primary machine
echo "Running commands on the primary machine..."
eval $PRIMARY_COMMANDS_VSYNC
sleep 2

# Run commands on the secondary machine via SSH
echo "Running commands on the secondary machine..."
ssh $SECONDARY_ADDRESS "$SECONDARY_COMMANDS_VSYNC" 2>&1 | tee $LOG_DIR$SECONDARY_LOG_VSYNC  &

cleanup() {
    echo "Cleaning up..."
    # Commands to stop ptp4l and phc2sys on the primary machine
    PRIMARY_STOP_COMMANDS_VSYNC="$SUDO kill -SIGINT \$(pgrep $VSYNCTEST) 2>/dev/null"

    # Commands to stop ptp4l and phc2sys on the secondary machine
    SECONDARY_STOP_COMMANDS_VSYNC="$SUDO kill -SIGINT \$(pgrep $VSYNCTEST) 2>/dev/null"

    # Stop commands on the secondary machine via SSH
    echo "Stopping commands on the secondary machine..."
    ssh $SECONDARY_ADDRESS "$SECONDARY_STOP_COMMANDS_VSYNC"

    # Stop commands on the primary machine
    echo "Stopping commands on the primary machine..."
    eval $PRIMARY_STOP_COMMANDS_VSYNC

    # Give some time for the processes to terminate
    sleep 1

    echo "Cleanup complete."
    echo "All processes terminated."
    exit 0
}

# Trap SIGINT (Ctrl+C) and call cleanup function
trap cleanup SIGINT

echo "Running... Press Ctrl+C to terminate."
while true; do
    sleep 1
done



