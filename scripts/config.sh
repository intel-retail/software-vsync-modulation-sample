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

#  Change/update values as per needed.


# Define command line parameters for ptp and vsync_test tools

SUDO="sudo"

# Primary machine configuration
PRIMARY_INERFACE="enpxxxx"
PRIMARY_ETH_ADDR="xx:xx:xx:xx:xx:xx"
PRIMARY_PROJ_PATH="<path on primary>/project" # Sample path

# Secondary machine configuration.
#IP needed for IP based communcation only
SECONDARY_ADDRESS="user@192.xxx.xx.xx"
SECONDARY_INTERFACE="enpxxxx"

EXECUTABLE_NAME="vsync_test"
# Path on secondary where the executable will be copied to.
# Update path accordingly. e.g user's home directory
CLIENT_DIR="<path on secondary>/.vblanksync"

# Output log files
PRIMARY_LOG_PTP="primary_ptp.log"
PRIMARY_LOG_PHC="primary_phc.log"
SECONDARY_LOG_PTP="secondary_ptp.log"
SECONDARY_LOG_PHC="secondary_phc.log"
PRIMARY_LOG_VSYNC="primary_vsync.log"
SECONDARY_LOG_VSYNC="secondary_vsync.log"

# Create logs directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR=$(dirname $SCRIPT_DIR)
mkdir -p $ROOT_DIR/logs
LOG_DIR="$ROOT_DIR/logs/"


