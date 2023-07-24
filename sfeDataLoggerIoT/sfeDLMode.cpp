



#include "sfeDLMode.h"
#include <string.h>

#include <Arduino.h>

// for mode word check in board eFuse memory
#include <soc/efuse_reg.h>

#include "mbedtls/aes.h"

// TODO - Testing things
uint8_t key[33] = "3Y3rOODfH4DV5XgpP/vkf6CHBZ2Rg3TI";
uint8_t key1[] = {51, 89, 51,  114, 79,  79, 68, 102, 72, 52, 68, 86, 53,  88, 103, 112,
                  80, 47, 118, 107, 102, 54, 67, 72,  66, 90, 50, 82, 103, 51, 84,  73};

#define IV_PREABLE "1111"


// what defines / IDs a IOT 9DOF board?
#define SFE_DL_IOT_9DOF_MODE (DL_MODE_FLAG_IMU | DL_MODE_FLAG_MAG | DL_MODE_FLAG_FUEL)

// Our generic - bare-bones IOT Board - 
#define SFE_DL_IOT_STD_MODE (DL_MODE_FLAG_FUEL | DL_MODE_FLAG_IOTSTD)

// define the boards we know about
static struct mode_entry{
	uint32_t 		mode;
	const char 	   *name;
	const char      prefix[5];
} dlBoardInfo[] = {
	// DataLogger 9DOF - ID Prefix [S]park[F]un [D]atalogger [1]
	{ SFE_DL_IOT_9DOF_MODE, "SparkFun DataLogger IoT - 9DoF", "SFD1"},
	{ SFE_DL_IOT_STD_MODE, "SparkFun DataLogger IoT", "SFD2"}
};


//---------------------------------------------------------------------------------
// do we know about this thing
bool dlModeCheckValid(uint32_t mode)
{
	// do we know about this board? 
	for(int i = 0; i < sizeof(dlBoardInfo)/sizeof(struct mode_entry); i++)
	{
		if ( (dlBoardInfo[i].mode & mode) == dlBoardInfo[i].mode )
			return true;
	}
	return false;
}

//---------------------------------------------------------------------------------
const char * dlModeCheckName(uint32_t mode)
{
	// do we know about this board? 
	for(int i = 0; i < sizeof(dlBoardInfo)/sizeof(struct mode_entry); i++)
	{
		if ( (dlBoardInfo[i].mode & mode) == dlBoardInfo[i].mode )
			return dlBoardInfo[i].name;
	}
	return "Unknown Board";

}
//---------------------------------------------------------------------------------
bool dlModeCheckPrefix(uint32_t mode, char prefix[5])
{
	// do we know about this board? 
	for(int i = 0; i < sizeof(dlBoardInfo)/sizeof(struct mode_entry); i++)
	{
		if ( (dlBoardInfo[i].mode & mode) == dlBoardInfo[i].mode )
		{
			memcpy(prefix, dlBoardInfo[i].prefix, 5);
			return true;
		}
	}
	return false;

}

//---------------------------------------------------------------------------------
// dlModeCheckSystem()
//
// Determine what board we are running on - and if it's a valid board.
//
// TODO: In progress

uint8_t dlModeCheckSystem(void)
{

	char szBuffer[64];

	// Get the contents of eFuse memory - BLOCK 3
	// Get the contents of eFuse BLOCK 3 - where the board info is flashed during production
    uint8_t data_buffer[32];
    int i = 0;
    for (int32_t block3Address = EFUSE_BLK3_RDATA0_REG; block3Address <= EFUSE_BLK3_RDATA7_REG; block3Address += 4)
    {
        uint32_t block = REG_GET_FIELD(block3Address, EFUSE_BLK3_DOUT0);

        // sprintf(h, "%02X %02X %02X %02X ", block & 0xFF, block >> 8 & 0xFF, block >> 16 & 0xFF, block >> 24 & 0xFF);
        // Serial.print(h);

        data_buffer[i++] = block & 0xFF;
        data_buffer[i++] = block >> 8 & 0xFF;
        data_buffer[i++] = block >> 16 & 0xFF;
        data_buffer[i++] = block >> 24 & 0xFF;
    }

// get the board ID
    uint64_t  chipID = ESP.getEfuseMac();
    uint8_t * pChipID = (uint8_t*)&chipID;

    // Build our IV 
    snprintf(szBuffer, sizeof(szBuffer), "%4s%02X%02X%02X%02X%02X%02X", IV_PREABLE, 
                    pChipID[0],pChipID[1],pChipID[2],pChipID[3],pChipID[4],pChipID[5]);

    uint8_t iv[16];
    memcpy(iv, szBuffer, sizeof(iv));

    // Decrypt time
    mbedtls_aes_context ctxAES;
    int rc = mbedtls_aes_setkey_dec(&ctxAES, key1, 256);
    if (rc != 0)
    {
        //Serial.println("Error with key length");
        return 0;
    }
    uint8_t output[32];
    memset(output, '\0', sizeof(output));

    rc = mbedtls_aes_crypt_cbc(&ctxAES, MBEDTLS_AES_DECRYPT, 32, iv, (unsigned char *)data_buffer,
                               (unsigned char *)output);

    mbedtls_aes_free(&ctxAES);
    if (rc != 0)
    {
        //Serial.println("error with decryption");
        return 0;
    }

    // Valid board?  - Does  the board ID match what was flashed on the board in production
    bool valid=true;

    for (int i=0; i < 6; i++)
    {
        if (output[3 + i] !=  pChipID[i])
        {
            //Serial.printf("Invalid board ID: %d flashed: %x, board: %x\n\r", i, output[3+i], pChipID[i]);
            valid = false;
            break;
        }
    }

    // todo - Check board type - look at first bytes: output[0]  - DataLogger, output[1] - Board ID



	return 0;
}
