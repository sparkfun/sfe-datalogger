

#pragma once

// Version information for the datalogger

// Major version number
#define kDLVersionNumberMajor		0

// Minor version number
#define kDLVersionNumberMinor		9

// Point version number
#define kDLVersionNumberPoint		1

// Version string description
#define kDLVersionDescriptor		"beta"


// app name/class ID string
#define kDLAppClassNameID			"SFE-DATALOGGER-IOT"

// Build number - should come from the build system. If not, set default

#ifndef BUILD_NUMBER
#define BUILD_NUMBER 0
#endif

