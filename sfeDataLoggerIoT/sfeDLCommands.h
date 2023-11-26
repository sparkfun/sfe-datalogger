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
#pragma once

#include "sfeDataLogger.h"
#include <ArduinoJson.h>

#include <Flux/flxSerialField.h>

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
