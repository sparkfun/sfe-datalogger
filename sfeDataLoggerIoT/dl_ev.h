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

#include <cstdint>


#define kEventNone    0

// Button Event Types ...
#define kEventButtonPress   1
#define kEventButtonRelease 2


void dl_evButtonEvent(uint32_t userButtonEvent);
bool dl_evButtonReset();