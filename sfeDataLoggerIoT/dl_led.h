
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

// LED Display for the datalogger - led display in a task
#pragma once

// calls to set/adjust LED display
void dl_ledActivity(bool immediate = false);
void dl_ledStartup(bool immediate = false);
void dl_ledBusy(bool immediate = false);
void dl_ledEditing(bool immediate = false);
void dl_ledOff(bool immediate = false);
bool dl_ledInit();
