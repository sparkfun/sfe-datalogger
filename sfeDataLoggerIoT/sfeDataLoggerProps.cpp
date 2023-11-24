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

/*
 * SparkFun Data Logger - Property Support Methods
 *
 */

#include "sfeDataLogger.h"

#include "sfeDLLed.h"

//---------------------------------------------------------------------------
// Property Callback methods for the application
//---------------------------------------------------------------------------
uint8_t sfeDataLogger::get_logTypeSD(void)
{
    return _logTypeSD;
}
//---------------------------------------------------------------------------
void sfeDataLogger::set_logTypeSD(uint8_t logType)
{
    if (logType == _logTypeSD)
        return;

    if (_logTypeSD == kAppLogTypeCSV)
        _fmtCSV.remove(&_theOutputFile);
    else if (_logTypeSD == kAppLogTypeJSON)
        _fmtJSON.remove(&_theOutputFile);

    _logTypeSD = logType;

    if (_logTypeSD == kAppLogTypeCSV)
        _fmtCSV.add(&_theOutputFile);
    else if (_logTypeSD == kAppLogTypeJSON)
        _fmtJSON.add(&_theOutputFile);
}
//---------------------------------------------------------------------------
uint8_t sfeDataLogger::get_logTypeSer(void)
{
    return _logTypeSer;
}
//---------------------------------------------------------------------------
void sfeDataLogger::set_logTypeSer(uint8_t logType)
{
    if (logType == _logTypeSer)
        return;

    if (_logTypeSer == kAppLogTypeCSV)
        _fmtCSV.remove(flxSerial());
    else if (_logTypeSer == kAppLogTypeJSON)
        _fmtJSON.remove(flxSerial());

    _logTypeSer = logType;

    if (_logTypeSer == kAppLogTypeCSV)
        _fmtCSV.add(flxSerial());
    else if (_logTypeSer == kAppLogTypeJSON)
        _fmtJSON.add(flxSerial());
}

//---------------------------------------------------------------------------
// json Buffer Size

uint sfeDataLogger::get_jsonBufferSize(void)
{
    return _fmtJSON.bufferSize();
}

void sfeDataLogger::set_jsonBufferSize(uint new_size)
{
    _fmtJSON.setBufferSize(new_size);
}

//---------------------------------------------------------------------------
// device names
//---------------------------------------------------------------------------
bool sfeDataLogger::get_verbose_dev_name(void)
{
    return flux.verboseDevNames();
}

void sfeDataLogger::set_verbose_dev_name(bool enable)
{
    flux.setVerboseDevNames(enable);
}

//---------------------------------------------------------------------------
// Sleep
//---------------------------------------------------------------------------
bool sfeDataLogger::get_sleepEnabled(void)
{
    return _bSleepEnabled;
}

//---------------------------------------------------------------------------
void sfeDataLogger::set_sleepEnabled(bool enabled)
{
    if (_bSleepEnabled == enabled)
        return;

    _bSleepEnabled = enabled;

    // manage our event -- if enabled, add to loop events, else remove it
    // Is it in the loop event list ?
    auto it = std::find(_loopEventList.begin(), _loopEventList.end(), &_sleepEvent);

    if (_bSleepEnabled)
    {
        _sleepEvent.last = millis();

        // just to be safe - check for dups
        if (it == _loopEventList.end())
            _loopEventList.push_back(&_sleepEvent);
    }
    else
    {
        if (it != _loopEventList.end()) // need to remove this
            _loopEventList.erase(it);
    }
}

//---------------------------------------------------------------------------
// Wake interval - get/set in secs; stored in our sleep event as MSecs
uint sfeDataLogger::get_sleepWakePeriod(void)
{
    return _sleepEvent.delta / 1000;
}
//---------------------------------------------------------------------------
// set period -- in secs
void sfeDataLogger::set_sleepWakePeriod(uint period)
{
    _sleepEvent.delta = period * 1000;
}

//---------------------------------------------------------------------------
// LED
//---------------------------------------------------------------------------
bool sfeDataLogger::get_ledEnabled(void)
{
    return !sfeLED.disabled();
}
//---------------------------------------------------------------------------
void sfeDataLogger::set_ledEnabled(bool enabled)
{
    sfeLED.setDisabled(!enabled);
}

//---------------------------------------------------------------------------
// Terminal Baudrate things
//---------------------------------------------------------------------------
uint sfeDataLogger::get_termBaudRate(void)
{
    return _terminalBaudRate;
}
//---------------------------------------------------------------------------
void sfeDataLogger::set_termBaudRate(uint newRate)
{
    // no change?
    if (newRate == _terminalBaudRate)
        return;

    _terminalBaudRate = newRate;

    // Was this done during an edit session?
    if (inOpMode(kDataLoggerOpEditing))
    {
        flxLog_N(F("\n\r\n\r\t[The new baud rate of %u takes effect when this device is restarted]"), newRate);
        delay(700);
        setOpMode(kDataLoggerOpPendingRestart);
    }
}
//---------------------------------------------------------------------------
uint sfeDataLogger::getTerminalBaudRate(void)
{
    // Do we have this block in storage? And yes, a little hacky with name :)
    flxStorageBlock *stBlk = _sysStorage.getBlock(((flxObject *)this)->name());

    if (!stBlk)
        return kDefaultTerminalBaudRate;

    uint theRate = 0;
    bool status = stBlk->read(serialBaudRate.name(), theRate);

    _sysStorage.endBlock(stBlk);

    return status ? theRate : kDefaultTerminalBaudRate;
}

//---------------------------------------------------------------------------
// local/board name things
std::string sfeDataLogger::get_local_name(void)
{
    return flux.localName();
}
//---------------------------------------------------------------------------

void sfeDataLogger::set_local_name(std::string name)
{
    flux.setLocalName(name);
}
