

#pragma once

// Version information for the datalogger

#include <stdint.h>
// Mode flags

// devices - on board 9DOF - flags
#define DL_MODE_FLAG_IMU (1 << 0)
#define DL_MODE_FLAG_MAG (1 << 1)
#define DL_MODE_FLAG_FUEL (1 << 2)


bool dlModeCheckValid(uint32_t mode);

const char * dlModeCheckName(uint32_t mode);

