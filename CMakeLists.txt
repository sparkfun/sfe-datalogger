cmake_minimum_required(VERSION 3.13)

# set project name - setting language to NONE disables the default compiler checks
project(cmake_test01 NONE)

# set our location of the SDK for now. set(FLUX_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/flux-sdk)

# call the sdk cmake system
include(flux_sdk_import.cmake)

# Where is directory that the flux stuff will be added to? This is the relative path from this file
# to the aurdiuno sketch directory this is also used as the name of the cmake project
flux_sdk_set_project_directory(.)

flux_sdk_set_platform(platform_esp32)
# Add some modules - note adding all the devices
flux_sdk_add_module(flux_all_modules)

# now call the init function/macro
flux_sdk_init()
