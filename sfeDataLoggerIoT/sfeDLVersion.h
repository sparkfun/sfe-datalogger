/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#pragma once

// Board ID/Mode information data

// Major version number
#define kDLVersionNumberMajor 1

// Minor version number
#define kDLVersionNumberMinor 2

// Point version number
#define kDLVersionNumberPoint 01

// Version string description
#define kDLVersionDescriptor "Version 1.2.01 Development"

// app name/class ID string
#define kDLAppClassNameID "SFE-DATALOGGER-IOT"

// Build number - should come from the build system. If not, set default

#ifndef BUILD_NUMBER
#define BUILD_NUMBER 0
#endif

#define kDLVersionBoardDesc "(c) 2023-2024 SparkFun Electronics"