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
import sys
import subprocess
import argparse
import random
from Crypto.Cipher import AES
from base64 import b64encode


#### HACK
from base64 import b64decode

test_key = '3Y3rOODfH4DV5XgpP/vkf6CHBZ2Rg3TI'

#### END HACK

## State and consts

# Board Class and  types 

_CLASS_DATALOGGER = 0x01

_CLASS_TYPE_DL_BASE = 0x01
_CLASS_TYPE_DL_9DOF = 0x02

# define our board name to type dict
_supported_boards = {
    "DLBASE"    : [_CLASS_DATALOGGER, _CLASS_TYPE_DL_BASE],
    "DL9DOF"    : [_CLASS_DATALOGGER, _CLASS_TYPE_DL_9DOF]
    }
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

#-----------------------------------------------------------------------------
# Return numeric codes for a given board
#
# Return None for uknown board
def get_board_code(args):

    if args.board in _supported_boards:
        return _supported_boards[args.board]

    return None

#-----------------------------------------------------------------------------
def fuse_process(args):


    # get the ID of the connected board

    chipID = getESPChipID(args.port)

    if len(chipID) == 0:
        print("Unable to determine board id number - is a DataLogger attached to this system?")
        return

    # get the board code
    boardCode = get_board_code(args)

    if boardCode == None:
        print("Invalid board type specified: {0}".format(args.board))
        return


    #print("Board ID Number: {0}".format(chipID))

    # random ints for padding  - we want a 32 byte value (256 bits)
    r1 = int(random.random() * 1000000000)
    r2 = int(random.random() * 1000000000)

    # Build our ID string - limit to 32 chars long or encrypt will dork
    sID = "{:002X}{:002X}00{:<12.12s}{:006X}{:006X}".format(boardCode[0], boardCode[1], chipID, r1, r2)[:32]

    print("ID IS: {0} - {1}".format(sID, len(sID)))

    # encrypt the ID string  - IV - a 1 padded chip ID
    iv = bytes("1111"+chipID, 'ascii')
    keyb = bytes(test_key, 'utf-8')
    cipher = AES.new(keyb, AES.MODE_CBC, iv)

    enc_SID = b64encode(cipher.encrypt(sID.encode('utf-8')))

    print("Encoded ID: {0}".format(enc_SID))

    ## testing - check the decode of this for now.
    cipher2 = AES.new(keyb, AES.MODE_CBC, iv)
    denc_SID = cipher2.decrypt(b64decode(enc_SID))
    print("Decoded ID: {0}".format(denc_SID.decode('utf-8')))

    #end testing

#-----------------------------------------------------------------------------
def main(argv=None):

    parser = argparse.ArgumentParser(description='SparkFun DataLogger IoT ID fuse utility')

    parser.add_argument(
        '--port', '-p',
        help='Serial port device',
        default=os.environ.get('ESPTOOL_PORT', None))

    parser.add_argument(
        '--board', '-b', dest='board',
        help="Board type name", choices=list(_supported_boards),
        default="DLBASE", type=str)

    args = parser.parse_args(argv)

    fuse_process(args)

#-----------------------------------------------------------------------------
def _main():
    #try:
    main()
    # except Exception as e:
    #     print('\nA fatal error occurred: %s' % e)
    #     sys.exit(2)


if __name__ == '__main__':
    _main()
