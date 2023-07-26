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
import tempfile



from .dl_log import info, warning, error, debug, init_logging, set_debug

from .dl_prefs import dlPrefs

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
#   port    - port the board is connected to. 
#
def get_esp_chip_id(port):


    chipID = bytes(0)

    # Make the call to esptool.py -- capture output

    args = ['esptool'] if os.name == 'nt' else ['esptool.py']
    args.append("--port")
    args.append(port)
    args.append("chip_id")

    try:
        results = subprocess.run(args, capture_output=True, shell=(os.name == 'nt'))

    except Exception as err:
        error("Error running esptool.py: {0}".format(str(err)))
        return chipID

    # The results
    if results.returncode != 0:
        error("Error running esptool.py: return code {0}".format(results.returncode))
        return chipID

    # The desired ID is part of the text output from the esptool command, so parse the results.
    # Note - we start from the last line of the output, and work back.
    
    output = results.stdout.splitlines()

    for line in reversed(output):

        # we need a normal line
        line = str(line, 'ascii')
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

    # return the value as bytes
    return bytes.fromhex(chipID)

#-----------------------------------------------------------------------------
# burn_esp_chip_id()
#
# Burned to passed in ID to block3 of the efuse on the esp32
#
# Parameters:
#   port        - port the board is connected to
#
#   idFilename  - the filename that contains the ID to burn..
#
def burn_esp_chip_id(port, idFilename):


    # Make the call to esptool.py -- capture output
    args = ['espefuse'] if os.name == 'nt' else ['espefuse.py']
    args.append('--port')
    args.append(port)
    args.append('summary')

    #TODO - Enable Capture output
    try:
        results = subprocess.run(args, capture_output=False, shell=(os.name == 'nt'))

    except Exception as err:
        error("Error running {0}: {1}".format(args[0], str(err)))
        return False

    # The results
    if results.returncode != 0:
        error("Error running {0}: return code {1}".format(args[0], results.returncode))
        return chipID

    # # testing - just dump output
    # print(str(results.stdout))

    return True

#-----------------------------------------------------------------------------
# Return numeric codes for a given board
#
# Return None for uknown board
def get_board_code():

    if dlPrefs['fuse_board'] in _supported_boards:
        return _supported_boards[dlPrefs['fuse_board']]

    return None

#-----------------------------------------------------------------------------
# fuseid_process()
#
# Build the ID and write to eFuse Block 3

def fuseid_process():

    # get the board code
    boardCode = get_board_code()

    if boardCode == None:
        error("Invalid board type specified: {0}".format(dlPrefs['fuse_board']))
        return

    # get the ID of the connected board
    chipID = get_esp_chip_id(dlPrefs['fuse_port'])

    if len(chipID) == 0:
        error("Unable to determine board id number - is a DataLogger attached to this system?")
        return

    if dlPrefs['debug']:
        debug("Board ID: {0}, len: {1}".format(chipID.hex().upper(), len(chipID)))

    ## Build the data to write to the board convert to byte array
    bID = boardCode[0].to_bytes(1, 'big') + boardCode[1].to_bytes(1, 'big') + (0).to_bytes(1, 'big') + chipID

    if dlPrefs['debug']:
        debug("Binary ID is 0x{0}, len: {1}".format(bID.hex().upper(), len(bID)))

    # pad the ID to 32 bits 
    bID_pad = pad(bID, AES.block_size * (2 if AES.block_size == 16 else 1 ))

    if dlPrefs['debug']:
        debug("Binary Padded ID is {0}, len: {1}".format(bID_pad.hex().upper(), len(bID_pad)))

    # encrypt the ID string  - IV - a 1 padded chip ID. NOTE this preable pad needs
    # to be the same in the C++ decode block in the firmware .
    sIV  = '1111' + chipID.hex().upper()
    iv = bytes(sIV, 'ascii')
    if dlPrefs['debug']:
        debug("IV: {0}".format(sIV))

    # take the encryption key and make sure it's a byte array
    keyb = bytes(dlPrefs['fuse_key'], 'utf-8')

    cipher = AES.new(keyb, AES.MODE_CBC, iv)
    enc_ID = cipher.encrypt(bID_pad)

    if dlPrefs['debug']:
        debug("After Encrypt: {0}, len: {1}".format(enc_ID.hex().upper(), len(enc_ID)))

        ## testing - check the decode of this for now.
        cipher2 = AES.new(keyb, AES.MODE_CBC, iv)
        denc_SID = cipher2.decrypt(enc_ID)
        debug("Decoded ID: {0}".format(denc_SID.hex().upper()))

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

    if dlPrefs['debug']:
        debug("FILENAME: {0}".format(tmp_name))

    ## TODO BURN!

    status = burn_esp_chip_id(dlPrefs['fuse_port'], tmp_name)

    # delete our data file
    os.remove(tmp_name)
    if dlPrefs['debug']:
        debug("Deleted temporary file: {0}".format(tmp_name))

    # done!
    info("ID Fuse burned to board {0} completed".format(chipID.hex().upper()))


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
        default=None, type=str)

    parser.add_argument(
        '--key', '-k', dest='key',
        help="Fuse encryption key",
        default=None, type=str)

    parser.add_argument(
        '--version', '-v', help="Print version", action='store_true')

    parser.add_argument("-d", "--debug", help='sets debug mode', action='store_true')

    parser.add_argument("-t", "--testing", help='Uses the test key', action='store_true')    

    argv = sys.argv[1:]
    args = parser.parse_args(argv)

    if args.version:
        print("{0}: version - {1}".format(os.path.basename(sys.argv[0]), dlPrefs['version']))
        sys.exit(0)

    if args.debug:
        dlPrefs['debug'] = True
        set_debug(True)
    else:
        dlPrefs['debug'] = False

    if args.port != None:
        dlPrefs['fuse_port'] = args.port

    if args.board != None:
        dlPrefs['fuse_board'] = args.board

    if args.key != None:
        dlPrefs['fuse_key'] = args.key

    # use the testing/development key?
    if args.testing:     
        info("Testing mode - using the test key for data encryption")
        dlPrefs['fuse_key'] = dlPrefs['fuse_test_key']

    # valudates our inputs
    if len(dlPrefs['fuse_key']) == 0:
        error("No fuse ID key provided. Exiting")
        sys.exit(1)

    if len(dlPrefs['fuse_port']) == 0:
        error("No serial port provided. Exiting")
        sys.exit(1)

    if len(dlPrefs['fuse_board']) == 0:
        error("No board type provided. Valid values: {0}. Exiting.".format(', '.join(_supported_boards)))
        sys.exit(1)

    fuseid_process()

#-----------------------------------------------------------------------------
def _main():
    #try:
    main()
    # except Exception as e:
    #     print('\nA fatal error occurred: %s' % e)
    #     sys.exit(2)


if __name__ == '__main__':
    _main()
