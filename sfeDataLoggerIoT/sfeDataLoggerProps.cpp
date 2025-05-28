/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

/*
 * SparkFun Data Logger - Property Support Methods
 *
 */

#include "sfeDLLed.h"
#include "sfeDLSystemOp.h"
#include "sfeDLVersion.h"
#include "sfeDataLogger.h"
#include <Flux/flxSerial.h>

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
        _fmtCSV.remove(flxSerial);
    else if (_logTypeSer == kAppLogTypeJSON)
        _fmtJSON.remove(flxSerial);

    _logTypeSer = logType;

    if (_logTypeSer == kAppLogTypeCSV)
        _fmtCSV.add(flxSerial);
    else if (_logTypeSer == kAppLogTypeJSON)
        _fmtJSON.add(flxSerial);
}

//---------------------------------------------------------------------------
// json Buffer Size

uint32_t sfeDataLogger::get_jsonBufferSize(void)
{
    return _fmtJSON.bufferSize();
}

void sfeDataLogger::set_jsonBufferSize(uint32_t new_size)
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

    if (_bSleepEnabled)
        flxAddJobToQueue(_sleepJob);
    else
        flxRemoveJobFromQueue(_sleepJob);
}

//---------------------------------------------------------------------------
// Wake interval - get/set in secs; stored in our sleep event as MSecs
uint32_t sfeDataLogger::get_sleepWakePeriod(void)
{
    return _sleepJob.period() / 1000;
}
//---------------------------------------------------------------------------
// set period -- in secs
void sfeDataLogger::set_sleepWakePeriod(uint32_t period)
{
    _sleepJob.setPeriod(period * 1000);
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
uint32_t sfeDataLogger::get_termBaudRate(void)
{
    return _terminalBaudRate;
}
//---------------------------------------------------------------------------
void sfeDataLogger::set_termBaudRate(uint32_t newRate)
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
void sfeDataLogger::getStartupProperties(uint32_t &baudRate, uint32_t &startupDelay)
{
    // Do we have this block in storage? And yes, a little hacky with name :)
    flxStorageBlock *stBlk = _sysStorage.getBlock(((flxObject *)this)->name());

    if (!stBlk)
    {
        baudRate = kDefaultTerminalBaudRate;
        startupDelay = kStartupMenuDefaultDelaySecs;
        return;
    }

    baudRate = 0;
    bool status = stBlk->read(serialBaudRate.name(), baudRate);

    if (!status)
        baudRate = kDefaultTerminalBaudRate;

    status = stBlk->read(startupDelaySecs.name(), startupDelay);
    if (!status)
        startupDelay = kStartupMenuDefaultDelaySecs;

    // Startup Output message level
    uint8_t uTmp;
    status = stBlk->read(startupOutputMode.name(), uTmp);
    startupOutputMode = (status ? uTmp : kAppStartupMsgNormal);

    _sysStorage.endBlock(stBlk);
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

//---------------------------------------------------------------------------
// Color text output
bool sfeDataLogger::get_color_text(void)
{
    return flxSerial.colorEnabled();
}
//---------------------------------------------------------------------------

void sfeDataLogger::set_color_text(bool enable)
{
    flxSerial.setColorEnabled(enable);
}
// for enabling system info in the log stream

bool sfeDataLogger::get_logsysinfo(void)
{
    return _bLogSysInfo;
}
void sfeDataLogger::set_logsysinfo(bool bEnableSysLog)
{
    // Change?
    if (bEnableSysLog == _bLogSysInfo)
        return;

    // Have we created an log object yet?
    if (!_pSystemInfo)
    {
        _pSystemInfo = new sfeDLSystemOp(this);
        if (!_pSystemInfo)
        {
            flxLogM_E(kMsgErrAllocError, "System Info Operator");
            return;
        }
    }
    _bLogSysInfo = bEnableSysLog;
    if (_bLogSysInfo)
        _logger.add(_pSystemInfo);
    else
        _logger.remove(_pSystemInfo);
}
//---------------------------------------------------------------------------
// verbose messages
//---------------------------------------------------------------------------
void sfeDataLogger::set_verbose(bool enable)
{

    // If disable, but we are in startup mode that enables verbose, don't set disable
    if (enable)
    {

        flxSetLoggingVerbose();

        // if in startup, the verbose mode is being set via pref restore. Note the change to user
        if (inOpMode(kDataLoggerOpStartup))
        {
            flxLog_N("");
            flxLog_V(F("Verbose output enabled"));
        }
    }
    else if (!inOpMode(kAppOpStartVerboseOutput))
        flxSetLoggingInfo();
}
bool sfeDataLogger::get_verbose(void)
{
    return flxIsLoggingVerbose();
}