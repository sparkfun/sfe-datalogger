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
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from base64 import b64encode
import tempfile



from .dl_log import info, warning, error, debug, init_logging, set_debug

from .dl_prefs import dlPrefs

test_key = '3Y3rOODfH4DV5XgpP/vkf6CHBZ2Rg3TI'

#### END HACK

#


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


    chipID = bytes(0)

    # Make the call to esptool.py -- capture output

    args = ['esptool.py']
    if port != None:
        args.append("--port")
        args.append(port)
    args.append("chip_id")


    try:
        results = subprocess.run(args, capture_output=True)

    except Exception as err:
        error("Error running esptool.py: {0}".format(str(err)))
        return chipID

    # The results
    if results.returncode != 0:
        error("Error running esptool.py: return code {0}".format(results.returncode))
        return chipID

    # The desired ID is part of the text output from the esptool command, so parse the results.
    # Note - we start from the last line of the output, and work back.

    output = str(results.stdout).split("\\n")

    for line in reversed(output):

        idx = line.find("Chip ID:")
        if idx == -1:
            idx = line.find("MAC:")
            if idx == -1:
                continue

        # we have an id - find the first :, then grab the ID values

        idx = line.find(":")
        tmpID = line[idx+1:]
        tmpID = tmpID.strip()  # cleanout whitespace

        # to match the ID we get at runtime, we need to - remove ":"

        chipID = ''.join(tmpID.split(':')).upper()

        break


    return bytes.fromhex(chipID)

#-----------------------------------------------------------------------------
# Return numeric codes for a given board
#
# Return None for uknown board
def get_board_code(args):

    if args.board in _supported_boards:
        return _supported_boards[args.board]

    return None

#-----------------------------------------------------------------------------
def fuseid_process(args):


    # get the board code
    boardCode = get_board_code(args)

    if boardCode == None:
        error("Invalid board type specified: {0}".format(args.board))
        return

    # get the ID of the connected board
    chipID = getESPChipID(args.port)

    if len(chipID) == 0:
        error("Unable to determine board id number - is a DataLogger attached to this system?")
        return

    debug("Board ID: {0}, len: {1}".format(chipID.hex().upper(), len(chipID)))

    ## Testing - convert to byte array
    bID = boardCode[0].to_bytes(1, 'big') + boardCode[1].to_bytes(1, 'big') + (0).to_bytes(1, 'big') + chipID

    debug("Binary ID is 0x{0}, len: {1}".format(bID.hex().upper(), len(bID)))

    bID_pad = pad(bID, AES.block_size * (2 if AES.block_size == 16 else 1 ))

    debug("Binary Padded ID is {0}, len: {1}".format(bID_pad.hex().upper(), len(bID_pad)))

    # encrypt the ID string  - IV - a 1 padded chip ID

    sIV  = '1111' + chipID.hex().upper()

    iv = bytes(sIV, 'ascii')
    debug("IV: {0}".format(sIV))

    keyb = bytes(test_key, 'utf-8')

    cipher = AES.new(keyb, AES.MODE_CBC, iv)
    enc_ID = cipher.encrypt(bID_pad)

    debug("After Encrypt: {0}, type: {1}, len: {2}".format(enc_ID.hex().upper(), type(enc_ID), len(enc_ID)))

    ## testing - check the decode of this for now.
    cipher2 = AES.new(keyb, AES.MODE_CBC, iv)
    denc_SID = cipher2.decrypt(enc_ID)
    debug("Decoded ID: {0}".format(denc_SID.hex().upper()))

    #end testing
    # pad the data to 32 - which is needed for
    # write the key to a temporary file
    tmp_name = ''
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat', prefix='dlfuse_') as f_tmp:
        tmp_name = f_tmp.name

    if len(tmp_name) == 0 :
        error("Error creating key data file. Unable to  continue")
        return

    f_tmp = open(tmp_name, 'wb')

    f_tmp.write(enc_ID)
    f_tmp.close()
    debug("FILENAME: {0}".format(tmp_name))

    ## TODO BURN!

    # delete our data file
    os.remove(tmp_name)

    # done!

    info("ID Fuse burn to board {0} completed".format(chipID.hex().upper()))


#-----------------------------------------------------------------------------
# do this...

init_logging()

#-----------------------------------------------------------------------------
def dl_fuseid():


    parser = argparse.ArgumentParser(description='SparkFun DataLogger IoT ID fuse utility')

    parser.add_argument(
        '--port', '-p',
        help='Serial port device',
        default=os.environ.get('ESPTOOL_PORT', None))

    parser.add_argument(
        '--board', '-b', dest='board',
        help="Board type name", choices=list(_supported_boards),
        default="DLBASE", type=str)

    parser.add_argument(
        '--version', '-v', help="Print version", action='store_true')

    parser.add_argument("-d", "--debug", help='sets debug mode', action='store_true')

    argv = sys.argv[1:]
    args = parser.parse_args(argv)

    if args.version:
        print("{0}: version - {1}".format(os.path.basename(sys.argv[0]), dlPrefs['version']))
        sys.exit(0)

    if args.debug:
        dlPrefs['debug'] = True
        set_debug(True)

    fuseid_process(args)

#-----------------------------------------------------------------------------
def _main():
    #try:
    main()
    # except Exception as e:
    #     print('\nA fatal error occurred: %s' % e)
    #     sys.exit(2)


if __name__ == '__main__':
    _main()
