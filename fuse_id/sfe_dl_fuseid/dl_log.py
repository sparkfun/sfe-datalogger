#--------------------------------------------------------------------------------
#
#
# Copyright (c) 2022-2024, SparkFun Electronics Inc.
#
# SPDX-License-Identifier: MIT
#
#
#--------------------------------------------------------------------------------

#
# Logging routines

# logging
import logging
import logging.handlers
import os
from pathlib import Path

from .dl_prefs import dlPrefs

s_console	= None
s_syslog	= None
s_logFile 	= None

_s_msg=None

#----------------------------------------------------------------------------------------
# initLogging()
#
# setup our logging system for this instance. This will initialize the logging and set it up to 
# output to the console
#
# Returns our logger

def init_logging():
	global s_console, s_syslog, _s_msg

	if _s_msg != None:
		return
		
	# note - we name our logger off our package name
	myLogger = logging.getLogger(dlPrefs['package_name'])
	
	if dlPrefs["debug"]:
		myLogger.setLevel(logging.DEBUG)
	else:
		myLogger.setLevel(logging.INFO)

	# define a Handler which writes INFO messages or higher to the sys.stderr
	if dlPrefs["log_console"]:
		s_console = logging.StreamHandler()
		formatter = logging.Formatter('%(levelname)s: %(message)s')
		s_console.setFormatter(formatter)
		
		# add the handler to the root logger
		myLogger.addHandler(s_console)
		myLogger.propagate=False

	# define a Handler which writes INFO messages or higher to the syslog
	if dlPrefs["log_syslog"]:
		s_syslog = logging.handlers.SysLogHandler(address = '/dev/log')
		formatter = logging.Formatter('%(asctime)s %(levelname)s: %(message)s')
		s_syslog.setFormatter(formatter)
		
		# add the handler to the root logger
		myLogger.addHandler(s_syslog)
		myLogger.propagate=False

	# define a Handler which writes INFO messages or higher to the syslog
	if dlPrefs["log_file"]:
		# determine location of log file
		logFilePath = str(Path.home()) + os.path.sep + 'Documents'
		if not os.path.exists(logFilePath):
			logFilePath = str(Path.home())
		logFileName = logFilePath + os.path.sep + dlPrefs['package_name'] + '.log'
		s_logFile = logging.handlers.TimedRotatingFileHandler(logFileName, when='midnight', interval=1, backupCount=4)
		formatter = logging.Formatter('%(asctime)s %(levelname)s: %(message)s')
		s_logFile.setFormatter(formatter)
		
		# add the handler to the root logger
		myLogger.addHandler(s_logFile)
		myLogger.propagate=False

	# better names
	logging.addLevelName(logging.INFO, "[I]")
	logging.addLevelName(logging.WARNING, "[W]")
	logging.addLevelName(logging.ERROR, "[E]")
	logging.addLevelName(logging.DEBUG, "[D]")
	_s_msg = myLogger

def set_debug(debug):

	myLogger = logging.getLogger(dlPrefs['package_name'])
	if debug:
		myLogger.setLevel(logging.DEBUG)
	else:
		myLogger.setLevel(logging.INFO)		
#----------------------------------------------------------------------------------------
# method to access our logger.
def getLogger():
	return _s_msg

def info(sMsg):
	_s_msg.info(sMsg)

def error(sMsg):
	_s_msg.error(sMsg)


def warning(sMsg):
	_s_msg.warning(sMsg)

def debug(sMsg):
	_s_msg.debug(sMsg)
