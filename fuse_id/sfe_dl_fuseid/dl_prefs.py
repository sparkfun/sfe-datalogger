

#--------------------------------------------------------------------------------
#
# Copyright (c) 2022-2023, SparkFun Electronics Inc.  All rights reserved.
# This software includes information which is proprietary to and a
# trade secret of SparkFun Electronics Inc.  It is not to be disclosed
# to anyone outside of this organization. Reproduction by any means
# whatsoever is  prohibited without express written permission.
#
#--------------------------------------------------------------------------------

# Purpose:
#   Preference system for the fuse command. Pretty simple.

from __future__ import print_function

import configobj
import validate
import os.path
import sys
from pathlib import Path


#what's our version
from .dl_version import VERSION

dlPrefs = {}

#------------------------------------------------------------------
# The plan:
#   Intialize our defaults and then see if we can load the user prefs
#------------------------------------------------------------------


# This file should be in the package  directory

dl_Home = os.path.dirname( os.path.realpath(__file__) )

defaultsFile = dl_Home + os.path.sep + '_dl_fuseid_.conf'



if(not os.path.isfile(defaultsFile)):
	print("ERROR: Unable to load system defaults. The sfe_dl_fuseid package is corrupt.", file=sys.stderr)

else:

	# To make this simple and cross platform - look for config file in home directory

	sysSettings = str(Path.home()) + os.path.sep + "dl_fuse.conf"
	if os.path.isfile(sysSettings):
		dlPrefs = configobj.ConfigObj(sysSettings, configspec=defaultsFile)
	else:
		dlPrefs = configobj.ConfigObj(configspec=defaultsFile)

	# validate the defaults
	valr = validate.Validator()
	retVal = dlPrefs.validate(valr)

	if retVal != True:
		bPass = False;
		# we've had some sort of errors. Let's detail this out
		for entry in configobj.flatten_errors(dlPrefs, retVal):
			# each entry is a tuple
			section_list, key, error = entry
		
			bPass=False
			if key is not None:

				section_list.append(key)
				section_string='default'
			else:
				section_list.append('[missing section]')
				section_string = ', '.join(section_list)
			if error == False:
				error = 'Missing value or section.'
		if not bPass:
			print('Error Reading preference file %s' % defaultsFile, file=sys.stderr)
			print(section_string, ' = ', error, file=sys.stderr)



# assume our parent dir is our pacakge name.
if __name__.find('.') > 0:
	dlPrefs['package_name'] = __name__.split('.')[-2]
else:
	dlPrefs['package_name'] = 'sfe_dl_fuseid'

dlPrefs['version'] = VERSION

dlPrefs['dl_fuseid_home'] = dl_Home
# at this point, we should be good 
