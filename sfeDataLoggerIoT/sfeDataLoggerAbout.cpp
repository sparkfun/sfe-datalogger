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
 * SparkFun Data Logger - about methods
 *
 */

#include "sfeDLMode.h"
#include "sfeDataLogger.h"
#include <Flux/flxSerial.h>

void sfeDataLogger::_displayAboutObjHelper(char pre_ch, const char *szName, bool enabled)
{
    flxLog_N_("%c    %-20s  : ", pre_ch, szName);
    if (enabled)
    {
        flxSerial.textToYellow();
        flxLog_N("enabled");
        flxSerial.textToNormal();
    }
    else
        flxLog_N("disabled");
}
//---------------------------------------------------------------------------
// "about"
void sfeDataLogger::displayAppStatus(bool useInfo)
{

    // type of output to use?
    flxLogLevel_t logLevel;
    char pre_ch;
    if (useInfo)
    {
        logLevel = flxLogInfo;
        pre_ch = ' ';
    }
    else
    {
        logLevel = flxLogNone;
        pre_ch = '\t';
    }

    // header
    if (!useInfo)
    {
        flxSerial.textToWhite();
        flxLog_N("    Time:");
        flxSerial.textToNormal();
    }
    time_t t_now;
    time(&t_now);
    struct tm *tmLocal = localtime(&t_now);

    char szBuffer[64];
    memset(szBuffer, '\0', sizeof(szBuffer));
    strftime(szBuffer, sizeof(szBuffer), "%G-%m-%dT%T", tmLocal);
    flxLog__(logLevel, "%cTime: %s", pre_ch, szBuffer);
    flxLog__(logLevel, "%cTime Zone: %s", pre_ch, flxClock.timeZone().c_str());

    // uptime
    uint32_t days, hours, minutes, secs, mills;

    flx_utils::uptime(days, hours, minutes, secs, mills);

    flxLog__(logLevel, "%cUptime: %u days, %02u:%02u:%02u.%u", pre_ch, days, hours, minutes, secs, mills);
    flxLog__(logLevel, "%cExternal Time Source: %s", pre_ch, flxClock.referenceClock().c_str());

    flxLog_N("");
    if (!useInfo)
    {
        flxSerial.textToWhite();
        flxLog_N("    Board:");
        flxSerial.textToNormal();
    }
    flxLog__(logLevel, "%cBoard Type: %s", pre_ch, dlModeCheckName(_modeFlags));
    flxLog__(logLevel, "%cBoard Name: %s", pre_ch, flux.localName().c_str());
    flxLog__(logLevel, "%cBoard ID: %s", pre_ch, flux.deviceId());

    flxLog_N("");

    if (!useInfo)
    {
        flxSerial.textToWhite();
        flxLog_N("    System:");
        flxSerial.textToNormal();
    }
    if (_theSDCard.enabled())
    {

        char szSize[32];
        char szCap[32];
        char szAvail[32];
        flx_utils::formatByteString(_theSDCard.size(), 2, szSize, sizeof(szSize));
        flx_utils::formatByteString(_theSDCard.total(), 2, szCap, sizeof(szCap));
        flx_utils::formatByteString(_theSDCard.total() - _theSDCard.used(), 2, szAvail, sizeof(szAvail));

        flxLog__(logLevel, "%cSD Card - Type: %s Size: %s Capacity: %s Free: %s (%.1f%%)", pre_ch, _theSDCard.type(),
                 szSize, szCap, szAvail, 100. - (_theSDCard.used() / (float)_theSDCard.total() * 100.));
    }
    else
        flxLog__(logLevel, "%cSD card not available", pre_ch);

    // show heap level
    flxLog__(logLevel, "%cSystem Heap - Total: %dB Free: %dB (%.1f%%)", pre_ch, ESP.getHeapSize(), ESP.getFreeHeap(),
             (float)ESP.getFreeHeap() / (float)ESP.getHeapSize() * 100.);

    if (_wifiConnection.enabled())
    {
        if (_wifiConnection.isConnected())
        {
            IPAddress addr = _wifiConnection.localIP();
            uint rating = _wifiConnection.rating();
            const char *szRSSI = rating == kWiFiLevelExcellent ? "Excellent"
                                 : rating == kWiFiLevelGood    ? "Good"
                                 : rating == kWiFiLevelFair    ? "Fair"
                                                               : "Weak";

            flxLog__(logLevel, "%cWiFi - Connected  SSID: %s  IP Address: %d.%d.%d.%d  Signal: %s", pre_ch,
                     _wifiConnection.connectedSSID().c_str(), addr[0], addr[1], addr[2], addr[3], szRSSI);
        }
        else
            flxLog__(logLevel, "%cWiFi - Not Connected", pre_ch);
    }
    else
        flxLog__(logLevel, "%cWiFi not enabled", pre_ch);

    // Battery fuel gauge available?
    if (_fuelGauge != nullptr)
    {
        // Output if a) we have a batter connected, and if so the % charge, and if it's charging
        float batterySOC = _fuelGauge->getSOC();
        // Is a battery connected - look at SOC
        if (batterySOC < kBatteryNoBatterySOC)
            flxLog__(logLevel, "%cBattery - Level: %c%.1f%%", pre_ch, _fuelGauge->getChangeRate() > 0 ? '+' : ' ',
                     batterySOC);
        else
            flxLog__(logLevel, "%cBattery - Not Connected", pre_ch);
    }
    flxLog__(logLevel, "%cSystem Deep Sleep: %s", pre_ch, sleepEnabled() ? "enabled" : "disabled");
    flxLog_N("%c    Sleep Interval: %d seconds", pre_ch, sleepInterval());
    flxLog_N("%c    Wake Interval: %d seconds", pre_ch, wakeInterval());

    flxLog_N("");
    if (!useInfo)
    {
        flxSerial.textToWhite();
        flxLog_N("    Logging:");
        flxSerial.textToNormal();
    }
    flxLog__(logLevel, "%cLogging Interval: %u (ms)", pre_ch, _timer.interval());

    // Run rate metric
    flxLog_N_("%c    Measured rate: ", pre_ch);
    if (useInfo || !_logger.enabledLogRate())
        flxLog_N("%s", (_logger.enabledLogRate() ? "<enabled>" : "<disabled>"));
    else
        flxLog_N("%.2f (ms)", _logger.getLogRate());

    flxLog__(logLevel, "%cJSON Buffer - Size: %dB Max Used: %dB", pre_ch, jsonBufferSize(), _fmtJSON.getMaxSizeUsed());
    flxLog__(logLevel, "%cSerial Output: %s", pre_ch, kLogFormatNames[serialLogType()]);
    flxLog_N("%c    Baud Rate: %d", pre_ch, serialBaudRate());
    flxLog__(logLevel, "%cSD Card Output: %s", pre_ch, kLogFormatNames[sdCardLogType()]);

    // at startup, useInfo == true, the file isn't known, so skip output
    if (!useInfo)
        flxLog_N("%c    Current Filename: \t%s", pre_ch,
                 _theOutputFile.currentFilename().length() == 0 ? "<none>" : _theOutputFile.currentFilename().c_str());
    flxLog_N("%c    Rotate Period: %d Hours", pre_ch, _theOutputFile.rotatePeriod());

    bool bEnabled = _extIntrEvent.isEnabled();
    flxLog__(logLevel, "%cInterrupt Log Trigger: %s", pre_ch, bEnabled ? "Enabled" : "Disabled");
    if (bEnabled)
    {
        flxLog__(logLevel, "%c    Pin: %d", pre_ch, _extIntrEvent.intrPin());
        flxLog__(logLevel, "%c    Event: %s", pre_ch, _extIntrEvent.eventName().c_str());
    }
    flxLog_N("");
    if (!useInfo)
    {
        flxSerial.textToWhite();
        flxLog_N("    GPIO:");
        flxSerial.textToNormal();
    }
    bEnabled = _extSerial.serialDeviceEnabled();
    flxLog__(logLevel, "%cSerial Device Logging: %s", pre_ch, bEnabled ? "Enabled" : "Disabled");
    if (bEnabled)
    {
        flxLog__(logLevel, "%c    RX Pin: %d", pre_ch, _extSerial.rxPin());
        flxLog__(logLevel, "%c    TX Pin: %d", pre_ch, _extSerial.txPin());
        flxLog__(logLevel, "%c    Baud Rate: %d", pre_ch, _extSerial.serialBaudRate());
    }
    bEnabled = _soilMoistureEnable.isEnabled();
    flxLog__(logLevel, "%cSoil Moisture Device: %s", pre_ch, bEnabled ? "Enabled" : "Disabled");
    if (bEnabled)
    {
        auto soilDevices = flux.get<flxDevSoilMoisture>();
        if (soilDevices->size() > 0)
        {
            flxDevSoilMoisture *pSoil = soilDevices->at(0);
            flxLog__(logLevel, "%c    VCC Pin: %d", pre_ch, pSoil->vccPin());
            flxLog__(logLevel, "%c    Sensor Pin: %d", pre_ch, pSoil->sensorPin());
        }
    }
    bEnabled = _analogPinEnable.isEnabled();
    flxLog__(logLevel, "%cAnalog Pin Sensor: %s", pre_ch, bEnabled ? "Enabled" : "Disabled");
    if (bEnabled)
    {
        auto analogDevices = flux.get<flxDevAnalogPin>();
        if (analogDevices->size() > 0)
            flxLog__(logLevel, "%c    Pin: %d", pre_ch, analogDevices->at(0)->sensorPin());
    }
    flxLog_N("");
    if (!useInfo)
    {
        flxSerial.textToWhite();
        flxLog_N(F("    IoT Services:"));
        flxSerial.textToNormal();
    }
    else
        flxLog__(logLevel, "%cIoT Services:", pre_ch);

    _displayAboutObjHelper(pre_ch, _mqttClient.name(), _mqttClient.enabled());
    _displayAboutObjHelper(pre_ch, _mqttSecureClient.name(), _mqttSecureClient.enabled());
    _displayAboutObjHelper(pre_ch, _iotHTTP.name(), _iotHTTP.enabled());
    _displayAboutObjHelper(pre_ch, _iotAWS.name(), _iotAWS.enabled());
    _displayAboutObjHelper(pre_ch, _iotAzure.name(), _iotAzure.enabled());
    _displayAboutObjHelper(pre_ch, _iotThingSpeak.name(), _iotThingSpeak.enabled());
    _displayAboutObjHelper(pre_ch, _iotMachineChat.name(), _iotMachineChat.enabled());
    _displayAboutObjHelper(pre_ch, _iotArduinoIoT.name(), _iotArduinoIoT.enabled());

    flxLog_N("");
    if (!useInfo)
    {
        flxSerial.textToWhite();
        flxLog_N("    Preview:");
        flxSerial.textToNormal();
    }
    else
        flxLog__(logLevel, "%cPreview:", pre_ch);
    _displayAboutObjHelper(pre_ch, _iotWebServer.name(), _iotWebServer.enabled());
    snprintf(szBuffer, sizeof(szBuffer), "mDNS: http://%s.local", _iotWebServer.mDNSName().c_str());
    _displayAboutObjHelper(pre_ch, szBuffer, _iotWebServer.mDNSEnabled());
    _displayAboutObjHelper(pre_ch, "Authentication", _iotWebServer.authUsername().length() > 0);

    flxLog_N("");

    // connected devices...
    flxDeviceContainer myDevices = flux.connectedDevices();
    if (!useInfo)
    {
        flxSerial.textToWhite();
        flxLog_N("    Connected Devices:");
        flxSerial.textToNormal();
    }
    else
        flxLog__(logLevel, "%cConnected Devices [%d]:", pre_ch, myDevices.size());

    // Loop over the device list - note that it is iterable.
    for (auto device : myDevices)
    {
        flxLog_N_(F("%c    %-20s  - %-40s  {"), pre_ch, device->name(), device->description());
        if (device->getKind() == flxDeviceKindI2C)
            flxLog_N("%s x%x}", "qwiic", device->address());
        else if (device->getKind() == flxDeviceKindSPI)
            flxLog_N("%s p%u}", "SPI", device->address());
        else if (device->getKind() == flxDeviceKindGPIO)
            flxLog_N("%s p%u}", "GPIO", device->address());
    }

    flxLog_N("");
}

//---------------------------------------------------------------------------
void sfeDataLogger::displayAppAbout()
{

    char szBuffer[128];
    flux.versionString(szBuffer, sizeof(szBuffer), true);

    flxSerial.textToWhite();
    flxLog_N("\n\r\t%s   %s", flux.name(), flux.description());
    flxLog_N("\tVersion: %s\n\r", szBuffer);
    flxSerial.textToNormal();

    displayAppStatus(false);
}
