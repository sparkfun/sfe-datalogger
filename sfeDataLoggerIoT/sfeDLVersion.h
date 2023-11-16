/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2023, SparkFun Electronics Inc.  All rights reserved.
 * This software includes information which is proprietary to and a
 * trade secret of SparkFun Electronics Inc.  It is not to be disclosed
 * to anyone outside of this organization. Reproduction by any means
 * whatsoever is  prohibited without express written permission.
 *
 *---------------------------------------------------------------------------------
 */

#pragma once

// Board ID/Mode information data

// Major version number
#define kDLVersionNumberMajor		1

// Minor version number
#define kDLVersionNumberMinor		1

// Point version number
#define kDLVersionNumberPoint		1

// Version string description
#define kDLVersionDescriptor		""


// app name/class ID string
#define kDLAppClassNameID			"SFE-DATALOGGER-IOT"

// Build number - should come from the build system. If not, set default

#ifndef BUILD_NUMBER
#define BUILD_NUMBER 0
#endif

#define kDLVersionBoardDesc 		"(c) 2023 SparkFun Electronics"