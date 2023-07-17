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

// Version information for the datalogger

#include <stdint.h>
// Mode flags

// devices - on board 9DOF - flags
#define DL_MODE_FLAG_IMU (1 << 0)
#define DL_MODE_FLAG_MAG (1 << 1)
#define DL_MODE_FLAG_FUEL (1 << 2)
#define DL_MODE_FLAG_IOTSTD (1 << 3)

// pin for our IoT std board check
#define kDLModeSTDCheckPin 35

bool dlModeCheckValid(uint32_t mode);
const char * dlModeCheckName(uint32_t mode);
bool dlModeCheckPrefix(uint32_t mode, char prefix[5]);

