



#include "sfeDLMode.h"
#include <string.h>


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

