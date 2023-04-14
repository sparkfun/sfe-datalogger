



#include "dl_mode.h"


// what defines / IDs a IOT 9DOF board?
#define SFE_DL_IOT_9DOF_MODE (DL_MODE_FLAG_IMU | DL_MODE_FLAG_MAG | DL_MODE_FLAG_FUEL)



// define the boards we know about
static struct mode_entry{
	uint32_t 		mode;
	const char 	   *name;
} dlBoardInfo[] = {
	{ SFE_DL_IOT_9DOF_MODE, "SparkFun DataLogger IoT - 9DoF"}
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

