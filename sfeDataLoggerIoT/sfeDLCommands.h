/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.  All rights reserved.
 * This software includes information which is proprietary to and a
 * trade secret of SparkFun Electronics Inc.  It is not to be disclosed
 * to anyone outside of this organization. Reproduction by any means
 * whatsoever is  prohibited without express written permission.
 *
 *---------------------------------------------------------------------------------
 */
#pragma once

#include "sfeDataLogger.h"
#include <ArduinoJson.h>

#include <Flux/flxCoreLog.h>
#include <Flux/flxSerialField.h>
#include <Flux/flxUtils.h>
#include <time.h>

class sfeDLCommands
{
    typedef bool (sfeDLCommands::*commandCB_t)(sfeDataLogger *);
    typedef std::map<std::string, commandCB_t> commandMap_t;

    //---------------------------------------------------------------------
    // Command Callbacks
    //---------------------------------------------------------------------
    bool factoryResetDevice(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        return dlApp->_sysUpdate.factoryResetDevice();
    }

    //---------------------------------------------------------------------
    bool resetDevice(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        // Need to prompt for an a-okay ...
        Serial.printf("\n\rClear and Restart Device? [Y/n]? ");
        uint8_t selected = dlApp->_serialSettings.getMenuSelectionYN();
        flxLog_N("");

        if (selected != 'y' || selected == kReadBufferTimeoutExpired || selected == kReadBufferExit)
        {
            flxLog_I(F("Aborting..."));
            return false;
        }

        return resetDeviceForced(dlApp);
    }
    //---------------------------------------------------------------------
    bool resetDeviceForced(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        dlApp->_sysStorage.resetStorage();
        flxLog_I(F("Settings Cleared"));

        dlApp->_sysUpdate.restartDevice();

        return true;
    }
    //---------------------------------------------------------------------
    bool clearDeviceSettings(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        // Need to prompt for an a-okay ...
        Serial.printf("\n\rClear Device Saved Settings? [Y/n]? ");
        uint8_t selected = dlApp->_serialSettings.getMenuSelectionYN();
        flxLog_N("");

        if (selected != 'y' || selected == kReadBufferTimeoutExpired || selected == kReadBufferExit)
        {
            flxLog_I(F("Aborting..."));
            return false;
        }
        return clearDeviceSettingsForced(dlApp);
    }

    //---------------------------------------------------------------------
    bool clearDeviceSettingsForced(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        dlApp->_sysStorage.resetStorage();
        flxLog_I(F("Settings Cleared"));

        return true;
    }
    //---------------------------------------------------------------------
    bool restartDevice(sfeDataLogger *dlApp)
    {
        if (dlApp)
            dlApp->_sysUpdate.restartDevicePrompt();

        return true;
    }
    //---------------------------------------------------------------------
    bool restartDeviceForced(sfeDataLogger *dlApp)
    {
        if (dlApp)
            dlApp->_sysUpdate.restartDevice();

        return true;
    }
    //---------------------------------------------------------------------
    bool aboutDevice(sfeDataLogger *dlApp)
    {
        if (dlApp)
            dlApp->displayAppAbout();
        return true;
    }
    //---------------------------------------------------------------------
    bool helpDevice(sfeDataLogger *dlApp)
    {
        flxLog_N(F("Available Commands:"));
        for (auto it : _commandMap)
            flxLog_N(F("   !%s"), it.first.c_str());

        return true;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief Reads JSON from the serial console - uses as input into the settings system
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool loadJSONSettings(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        // Create a JSON prefs serial object and read in the settings
        flxStorageJSONPrefSerial prefsSerial(flxSettings.fallbackBuffer() > 0 ? flxSettings.fallbackBuffer() : 2000);

        // restore the settings from serial
        bool status = flxSettings.restoreObjectFromStorage(&flux, &prefsSerial);
        if (!status)
            return false;

        flxLog_I_(F("Settings restored from serial..."));

        // now save the new settings in primary storage
        status = flxSettings.save(&flux, true);
        if (status)
            flxLog_N(F("saved locally"));

        return status;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief Saves the current system to preferences/Settings
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool saveSettings(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        // Just call save
        bool status = flxSettings.save(&flux);
        if (status)
            flxLog_I(F("Saving System Settings."));
        else
            flxLog_E(F("Error saving settings"));

        return status;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief Dumps out the current heap size/stats
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool heapStatus(sfeDataLogger *dlApp)
    {
        // just dump out the current heap
        flxLog_I(F("System Heap - Total: %dB Free: %dB (%.1f%%)"), ESP.getHeapSize(), ESP.getFreeHeap(),
                 (float)ESP.getFreeHeap() / (float)ESP.getHeapSize() * 100.);
        return true;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief Enables *normal* log level output
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool logLevelNormal(sfeDataLogger *dlApp)
    {
        flxLog.setLogLevel(flxLogInfo);
        flxLog_I(F("Output level set to Normal"));
        return true;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief Enables debug log level output
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool logLevelDebug(sfeDataLogger *dlApp)
    {
        flxLog.setLogLevel(flxLogDebug);
        flxLog_D(F("Output level set to Debug"));
        return true;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief Enables verbose log level output
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool logLevelVerbose(sfeDataLogger *dlApp)
    {
        flxLog.setLogLevel(flxLogVerbose);
        flxLog_V(F("Output level set to Verbose"));
        return true;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief Dumps out the current logging rate metric
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool logRateStats(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        // Run rate metric
        flxLog_N_(F("Logging Rate - Set Interval: %u (ms)  Measured: "), dlApp->_timer.interval());
        if (!dlApp->_logger.enabledLogRate())
            flxLog_N("%s", "<disabled>");
        else
            flxLog_N("%.2f (ms)", dlApp->_logger.getLogRate());

        return true;
    }

    //---------------------------------------------------------------------
    ///
    /// @brief Toggles the state of current logging rate metric
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool logRateToggle(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        dlApp->_logger.logRateMetric = !dlApp->_logger.logRateMetric();
        // Run rate metric
        flxLog_N(F("Logging Rate Metric %s"), dlApp->_logger.enabledLogRate() ? "Enabled" : "Disabled");

        return true;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief Dumps out the current wifi stats
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool wifiStats(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        if (dlApp->_wifiConnection.enabled() && dlApp->_wifiConnection.isConnected())
        {
            IPAddress addr = dlApp->_wifiConnection.localIP();
            uint rating = dlApp->_wifiConnection.rating();
            const char *szRSSI = rating == kWiFiLevelExcellent ? "Excellent"
                                 : rating == kWiFiLevelGood    ? "Good"
                                 : rating == kWiFiLevelFair    ? "Fair"
                                                               : "Weak";
            flxLog_I(F("WiFi - Connected  SSID: %s  IP Address: %d.%d.%d.%d  Signal: %s"),
                     dlApp->_wifiConnection.connectedSSID().c_str(), addr[0], addr[1], addr[2], addr[3], szRSSI);
        }
        else
            flxLog_I(F("WiFi - Not Connected/Enabled"));

        return true;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief Dumps out the current sd card stats
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool sdCardStats(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        if (dlApp->_theSDCard.enabled())
        {

            char szSize[32];
            char szCap[32];
            char szAvail[32];

            flx_utils::formatByteString(dlApp->_theSDCard.size(), 2, szSize, sizeof(szSize));
            flx_utils::formatByteString(dlApp->_theSDCard.total(), 2, szCap, sizeof(szCap));
            flx_utils::formatByteString(dlApp->_theSDCard.total() - dlApp->_theSDCard.used(), 2, szAvail,
                                        sizeof(szAvail));

            flxLog_I(F("SD Card - Type: %s Size: %s Capacity: %s Free: %s (%.1f%%)"), dlApp->_theSDCard.type(), szSize,
                     szCap, szAvail, 100. - (dlApp->_theSDCard.used() / (float)dlApp->_theSDCard.total() * 100.));
        }
        else
            flxLog_I(F("SD card not available"));

        return true;
    }

    //---------------------------------------------------------------------
    ///
    /// @brief Lists loaded devices
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool listLoadedDevices(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        // connected devices...
        flxDeviceContainer myDevices = flux.connectedDevices();
        flxLog_I(F("Connected Devices [%d]:"), myDevices.size());

        // Loop over the device list - note that it is iterable.
        for (auto device : myDevices)
        {
            flxLog_N_(F("    %-20s  - %s  {"), device->name(), device->description());
            if (device->getKind() == flxDeviceKindI2C)
                flxLog_N("%s x%x}", "qwiic", device->address());
            else
                flxLog_N("%s p%u}", "SPI", device->address());
        }

        return true;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief outputs current time
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool outputSystemTime(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        char szBuffer[64];
        memset(szBuffer, '\0', sizeof(szBuffer));
        time_t t_now;
        time(&t_now);

        flx_utils::timestampISO8601(t_now, szBuffer, sizeof(szBuffer), true);

        flxLog_I("%s", szBuffer);

        return true;
    }

    //---------------------------------------------------------------------
    ///
    /// @brief outputs uptime
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool outputUpTime(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        // uptime
        uint32_t days, hours, minutes, secs, mills;

        flx_utils::uptime(days, hours, minutes, secs, mills);

        flxLog_I("Uptime: %u days, %02u:%02u:%02u.%u", days, hours, minutes, secs, mills);

        return true;
    }
    //---------------------------------------------------------------------
    ///
    /// @brief log an observation now!
    ///
    /// @param dlApp Pointer to the DataLogger App
    /// @retval bool indicates success (true) or failure (!true)
    ///
    bool logObservationNow(sfeDataLogger *dlApp)
    {
        if (!dlApp)
            return false;

        dlApp->_logger.logObservation();

        return true;
    }
    //---------------------------------------------------------------------
    // our command map - command name to callback method
    commandMap_t _commandMap = {
        {"factory-reset", &sfeDLCommands::factoryResetDevice},
        {"reset-device", &sfeDLCommands::resetDevice},
        {"reset-device-forced", &sfeDLCommands::resetDeviceForced},
        {"clear-settings", &sfeDLCommands::clearDeviceSettings},
        {"clear-settings-forced", &sfeDLCommands::clearDeviceSettingsForced},
        {"restart", &sfeDLCommands::restartDevice},
        {"restart-forced", &sfeDLCommands::restartDeviceForced},
        {"json-settings", &sfeDLCommands::loadJSONSettings},
        {"log-rate", &sfeDLCommands::logRateStats},
        {"log-rate-toggle", &sfeDLCommands::logRateToggle},
        {"log-now", &sfeDLCommands::logObservationNow},
        {"wifi", &sfeDLCommands::wifiStats},
        {"sdcard", &sfeDLCommands::sdCardStats},
        {"devices", &sfeDLCommands::listLoadedDevices},
        {"save-settings", &sfeDLCommands::saveSettings},
        {"heap", &sfeDLCommands::heapStatus},
        {"normal-output", &sfeDLCommands::logLevelNormal},
        {"debug-output", &sfeDLCommands::logLevelDebug},
        {"verbose-output", &sfeDLCommands::logLevelVerbose},
        {"systime", &sfeDLCommands::outputSystemTime},
        {"uptime", &sfeDLCommands::outputUpTime},
        {"about", &sfeDLCommands::aboutDevice},
        {"help", &sfeDLCommands::helpDevice},
    };

  public:
    bool processCommand(sfeDataLogger *dlApp)
    {
        // The data editor we're using - serial field
        flxSerialField theDataEditor;
        std::string sBuffer;
        bool status = theDataEditor.editFieldString(sBuffer);

        flxLog_N(""); // need to output a CR

        if (!status)
            return false;

        // cleanup string
        sBuffer = flx_utils::strtrim(sBuffer);

        // Find our command
        commandMap_t::iterator it = _commandMap.find(sBuffer);
        if (it != _commandMap.end())
            status = (this->*(it->second))(dlApp);
        else
        {
            flxLog_N(F("Unknown Command: %s"), sBuffer.c_str());
            status = false;
        }
        return status;
    }
};
