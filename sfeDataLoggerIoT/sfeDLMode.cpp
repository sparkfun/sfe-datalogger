/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include "sfeDLMode.h"
#include <string.h>

#include <Esp.h>
#include <stdio.h>

// for mode word check in board eFuse memory
#include <soc/efuse_reg.h>

#include "mbedtls/aes.h"

// Our ID key - we have two keys - one used during development and one for production
//
// When doing development, ID a board using the '-t' option to the id_fuseid command
//

// If a key array is passed in via a #define, use that, otherwise use a default, dev key
#ifdef DATALOGGER_IOT_ID_KEY
static const uint8_t _app_mode[] = {DATALOGGER_IOT_ID_KEY};
#else
static const uint8_t _app_mode[] = {51, 89, 51,  114, 79,  79, 68, 102, 72, 52, 68, 86, 53,  88, 103, 112,
                                    80, 47, 118, 107, 102, 54, 67, 72,  66, 90, 50, 82, 103, 51, 84,  73};
#endif

#define IV_PREABLE "1111"

// For BLOCK 3 ID work
#define _BLOCK3_ID_DL_IOT 0x1

// BLOCK 3 BOARD CODES
#define _BLOCK3_ID_DL_IOT_BASIC 0x01
#define _BLOCK3_ID_DL_IOT_9DOF 0x02

// what defines / IDs a IOT 9DOF board?
#define SFE_DL_IOT_9DOF_MODE_DEV (DL_MODE_FLAG_IMU | DL_MODE_FLAG_MAG | DL_MODE_FLAG_FUEL)

// Define flags for supported boards. The mode word reserves bits 4->15 for this.
#define SFE_DL_IOT_VALID_MASK (SFE_DL_IOT_STD_MODE | SFE_DL_IOT_9DOF_MODE)

// define the boards we know about
static struct mode_entry
{
    uint32_t mode;
    const char *name;
    const char prefix[5];
} dlBoardInfo[] = {
    // DataLogger 9DOF - ID Prefix [S]park[F]un [D]atalogger [1]
    {SFE_DL_IOT_9DOF_MODE, "SparkFun DataLogger IoT - 9DoF", "SFD1"},
    {SFE_DL_IOT_STD_MODE, "SparkFun DataLogger IoT", "SFD2"}};

//---------------------------------------------------------------------------------
// do we know about this thing
bool dlModeCheckValid(uint32_t mode)
{
    // it's a valid board if it has a bit in the valid Mask
    return (mode & SFE_DL_IOT_VALID_MASK) != 0;
}

//---------------------------------------------------------------------------------
const char *dlModeCheckName(uint32_t mode)
{
    // do we know about this board?
    for (int i = 0; i < sizeof(dlBoardInfo) / sizeof(struct mode_entry); i++)
    {
        if ((dlBoardInfo[i].mode & mode) == dlBoardInfo[i].mode)
            return dlBoardInfo[i].name;
    }
    return "Unknown Board";
}
//---------------------------------------------------------------------------------
bool dlModeCheckPrefix(uint32_t mode, char prefix[5])
{
    // do we know about this board?
    for (int i = 0; i < sizeof(dlBoardInfo) / sizeof(struct mode_entry); i++)
    {
        if ((dlBoardInfo[i].mode & mode) == dlBoardInfo[i].mode)
        {
            memcpy(prefix, dlBoardInfo[i].prefix, 5);
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------------
// dlModeCheckDeviceMode
//
// The original board used devices to determine if the board was valid. This really
// didn't scale, but the firmware needs to support it.
//
// This method will set the appropriate mode bits if the devices for the 9DOF board
// are present.
//
// Returns true if a 9DOF board and the mode needs to be updated

bool dlModeCheckDevice9DOF(uint32_t inMode)
{
    if (dlModeCheckValid(inMode))
        return false;

    // In valid board - do we have all the 9DOF devices on board?
    return ((SFE_DL_IOT_9DOF_MODE_DEV & inMode) == SFE_DL_IOT_9DOF_MODE_DEV);
}
//---------------------------------------------------------------------------------
// dlModeCheckSystem()
//
// Determine what board we are running on - and if it's a valid board.
//
// TODO: In progress
//
// Return Value:
//    0  - Zero of nothing is valid
//       - Device Mode bit of the detected device.

uint32_t dlModeCheckSystem(void)
{

    char szBuffer[64];

    // Get the contents of eFuse BLOCK 3 - where the board info is flashed during production
    uint8_t data_buffer[32];
    int i = 0;
    for (int32_t block3Address = EFUSE_BLK3_RDATA0_REG; block3Address <= EFUSE_BLK3_RDATA7_REG; block3Address += 4)
    {
        // this is read in 4 byte chunks...
        uint32_t block = REG_GET_FIELD(block3Address, EFUSE_BLK3_DOUT0);

        data_buffer[i++] = block & 0xFF;
        data_buffer[i++] = block >> 8 & 0xFF;
        data_buffer[i++] = block >> 16 & 0xFF;
        data_buffer[i++] = block >> 24 & 0xFF;
    }

    // get the board ID
    uint64_t chipID = ESP.getEfuseMac();
    uint8_t *pChipID = (uint8_t *)&chipID;

    // Build our IV
    snprintf(szBuffer, sizeof(szBuffer), "%4s%02X%02X%02X%02X%02X%02X", IV_PREABLE, pChipID[0], pChipID[1], pChipID[2],
             pChipID[3], pChipID[4], pChipID[5]);

    uint8_t iv[16];
    memcpy(iv, szBuffer, sizeof(iv));

    // Decrypt time
    mbedtls_aes_context ctxAES;
    int rc = mbedtls_aes_setkey_dec(&ctxAES, _app_mode, 256);
    if (rc != 0)
    {
        // Serial.println("Error with key length");
        return 0;
    }
    uint8_t output[32];
    memset(output, '\0', sizeof(output));

    rc = mbedtls_aes_crypt_cbc(&ctxAES, MBEDTLS_AES_DECRYPT, 32, iv, (unsigned char *)data_buffer,
                               (unsigned char *)output);

    mbedtls_aes_free(&ctxAES);
    if (rc != 0)
    {
        // Serial.println("error with decryption");
        return 0;
    }

    // Valid board?  - Does  the board ID match what was flashed on the board in production

    for (int i = 0; i < 6; i++)
    {
        if (output[3 + i] != pChipID[i])
        {
            // Not valid - return 0
            return 0;
        }
    }

    // Okay , ID match is solid, check type flags - is it a Datalogger?

    if (output[0] & _BLOCK3_ID_DL_IOT != _BLOCK3_ID_DL_IOT)
        return 0;

    switch (output[1])
    {
    case _BLOCK3_ID_DL_IOT_BASIC:
        return SFE_DL_IOT_STD_MODE;

    case _BLOCK3_ID_DL_IOT_9DOF:
        return SFE_DL_IOT_9DOF_MODE;

    default:
        break;
    }

    // if we are here, this is an unknown board.
    return 0;
}
