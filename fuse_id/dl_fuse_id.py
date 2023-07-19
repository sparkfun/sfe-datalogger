#!/usr/bin/env python
#
#---------------------------------------------------------------------------------
#
# Copyright (c) 2022-2023, SparkFun Electronics Inc.  All rights reserved.
# This software includes information which is proprietary to and a
# trade secret of SparkFun Electronics Inc.  It is not to be disclosed
# to anyone outside of this organization. Reproduction by any means
# whatsoever is  prohibited without express written permission.
#
#---------------------------------------------------------------------------------
#
#
# Overview
#   Command line too to burn the board type and ID into a eFuse memory block on
#   the ESP32 MCU of a DataLogger IoT board.
#
#   Since the firmware on the Datalogger spans a number of boards, a board type
#   code is flashed into the eFuse OTP memory on the ESP32.
#
#   Additionally the chip id/MAC address of the ESP32 is flashed into this memory
#
#   Both the type code and ID are AES encrypted using a provided KEY, before
#   being burned to eFuse BLOCK 3 of the ESP32.
#
#   On startup, the DataLogger firmware will access the contents of the eFuse
#   block three OTP memory, dycrypt it and a) determine the board type from
#   the included board type code, and b) verify the chip id in the OTP memory
#   matches the chip ID of the board.
#
# Dependancy Notes:
#   This untility depends on the install of the following tools from espressif:
#       - esptool.py
#       - espefuse.py
#
#   This tool will launch sub-processes to execute these commands.
#
#   These tools should be installed before the use of this tool.
#
#-----------------------------------------------------------------------------
#
# pylint: disable=old-style-class, missing-docstring, wrong-import-position
#
#-----------------------------------------------------------------------------
# Imports

import time
import os
import os.path
import subprocess


#-----------------------------------------------------------------------------
# get_esp_chip_id()
#
# Return the chip id of the attached ESP32 board as a string. If not found, an empty string is returned
#
# Parameters:
#   port    - port the board is connected to
#
def getESPChipID(port=None):

    # Make the call to esptool.py -- capture output

    args = ['esptool.py']
    if port != None:
        args.append("--port")
        args.append(port)
    args.append("chip_id")


    try:
        results = subprocess.run(args, capture_output=True)

    except Exception as err:
        print("Error running esptool.py: {0}".format(str(err)))
        return ""

    # The results
    if results.returncode != 0:
        print("Error running esptool.py: return code {0}".format(results.returncode))
        return ""

    # The desired ID is part of the text output from the esptool command, so parse the results.
    # Note - we start from the last line of the output, and work back.

    output = str(results.stdout).split("\\n")

    chipID = ""

    for line in reversed(output):

        idx = line.find("Chip ID:")
        if idx == -1:
            idx = line.find("MAC:")
            if idx == -1:
                continue

        # we have an id - find the first :, then grab the ID values

        idx = line.find(":")
        tmpID = line[idx+1:]
        chipID = tmpID.strip()  # cleanout whitespace

        # to match the ID we get at runtime, we need to - remove ":" and reverse the order of the address tuples

        tups = chipID.split(':')
        tups.reverse()
        ''.join(tups)
        chipID = ''.join(tups).upper()

        break


    return chipID
