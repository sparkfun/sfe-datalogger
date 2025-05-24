#
# Copyright (c) 2022-2024, SparkFun Electronics Inc.
#
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.13)

# set project name - setting language to NONE disables the default compiler checks
project(DataLoggerIoT NONE)

# Import the flux-dk cmake system
include($ENV{FLUX_SDK_PATH}/external/flux_sdk_import.cmake)

# set our Arduino Library name to something unique to this project
flux_sdk_set_library_name(SparkFun_DataLoggerIoT)

# Where is directory that the flux stuff will be added to? This is the relative path from this file
# to the aurdiuno sketch directory this is also used as the name of the cmake project
flux_sdk_set_project_directory(.)

# datalogger is esp32 based
flux_sdk_set_platform(platform_esp32)

# Currently we are using all modules of the SDK
flux_sdk_add_module(flux_all_modules)

# now call the init function/macro - this will build the Arduino Library SparkFun_Flux under this
# main directory
flux_sdk_init()
