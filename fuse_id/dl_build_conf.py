#!/usr/bin/env python

import os
import sys
import datetime


if 'DATALOGGER_IOT_ID_KEY' not in os.environ:
    print("SparkFun Production KEY SECRET not available. Exiting")
    sys.exit(1)



# parse the key array and create our string key value

key_data = os.environ['DATALOGGER_IOT_ID_KEY']
key = ''
for nn in key_data.split(','):
    key = key + chr(int(nn))


# lets write out a test file for this

with open('dl_fuseid.conf', 'w') as f:
    f.write("# SparkFun - DataLogger IoT fuse production key\n")
    f.write("#\n")
    f.write("# {0}\n".format(datetime.datetime.now()))
    f.write("# Repository: {0}\n".format(os.environ["GITHUB_REPOSITORY"]))
    f.write("# Build Number: {0}\n".format(os.environ["GITHUB_RUN_NUMBER"]))
    f.write("\n")
    f.write("fuse_key= {0}\n".format(key))
    f.write("\n")
    f.write("# Enter the com port for the device below\n")
    f.write("fuse_port=\n")



sys.exit(0)
