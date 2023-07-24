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


// Define flags for supported boards. The mode word reserves bits 4->15 for this. 

// Standard DataLogger
#define SFE_DL_IOT_STD_MODE     (1 << 4)

// Our 9DOF DataLogger 
#define SFE_DL_IOT_9DOF_MODE    (1 << 5)

// Is this board of a specific type?
#define dlModeIsBoardType(__type__, __mode__)  ( (__type__ & __mode__) == __type__)

#define dlModeIsDLBasicBoard(__mode__)  ( dlModeIsBoardType(SFE_DL_IOT_STD_MODE, __mode__))
#define dlModeIsDL9DOFBoard(__mode__)  ( dlModeIsBoardType(SFE_DL_IOT_9DOF_MODE, __mode__))

bool dlModeCheckValid(uint32_t mode);
const char * dlModeCheckName(uint32_t mode);
bool dlModeCheckPrefix(uint32_t mode, char prefix[5]);
bool dlModeCheckDevice9DOF(uint32_t inMode);

uint32_t dlModeCheckSystem(void);

