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

/*
 * Spark Framework demo - logging
 *
 */
#pragma once

// Spark framework
#include <Flux.h>
#include <Flux/flxCoreJobs.h>
#include <Flux/flxCoreLog.h>
#include <Flux/flxFmtCSV.h>
#include <Flux/flxFmtJSON.h>
#include <Flux/flxLogger.h>
#include <Flux/flxSerial.h>
#include <Flux/flxTimer.h>

// settings storage
#include <Flux/flxSettings.h>
#include <Flux/flxSettingsSerial.h>
#include <Flux/flxStorageESP32Pref.h>
#include <Flux/flxStorageJSONPref.h>

// SD Card output
#include <Flux/flxFSSDMMCard.h>
#include <Flux/flxFileRotate.h>

// WiFi and NTP
#include <Flux/flxNTPESP32.h>
#include <Flux/flxWiFiESP32.h>

// NFC device
#include <Flux/flxDevST25DV.h>

// SPI Devices
// The onboard IMU
#include <Flux/flxDevISM330.h>

// The onboard Magnetometer
#include <Flux/flxDevMMC5983.h>

// Biometric Hub -- requires pins to be set on startup
#include <Flux/flxDevBioHub.h>

// Fuel gauge
#include <Flux/flxDevMAX17048.h>

// IoT Client Includes
#include <Flux/flxIoTAWS.h>
#include <Flux/flxIoTArduino.h>
#include <Flux/flxIoTAzure.h>
#include <Flux/flxIoTHTTP.h>
#include <Flux/flxIoTMachineChat.h>
#include <Flux/flxIoTThingSpeak.h>
#include <Flux/flxMQTTESP32.h>

// System Firmware update/reset
#include <Flux/flxSysFirmware.h>

// OLED
//#include <Flux/flxDevMicroOLED.h>

#include "sfeDLButton.h"
#include "sfeDLWebServer.h"

// #ifdef ENABLE_OLED_DISPLAY
// #include "sfeDLDisplay.h"
// #endif

#include <utility>

//------------------------------------------
// Default log interval in milli secs
const uint16_t kDefaultLogInterval = 15000;

// Buffer size of our JSON document output
const uint16_t kAppJSONDocSize = 1600;

// NTP Startup delay in secs
const uint16_t kAppNTPStartupDelaySecs = 5;

// Battery check interval (90 seconds)
const uint32_t kBatteryCheckInterval = 90000;

// Default Sleep Periods
const uint16_t kSystemSleepSleepSec = 60;
const uint16_t kSystemSleepWakeSec = 120;

// What is the out of the box baud rate ..
const uint32_t kDefaultTerminalBaudRate = 115200;

// General startup delay (secs) for the startup menu
const uint32_t kStartupMenuDefaultDelaySecs = 2;

// Operation mode flags
#define kDataLoggerOpNone (0)
#define kDataLoggerOpEditing (1 << 0)
#define kDataLoggerOpStartup (1 << 1)
#define kDataLoggerOpPendingRestart (1 << 2)

// startup things
#define kDataLoggerOpStartNoAutoload (1 << 3)
#define kDataLoggerOpStartListDevices (1 << 4)
#define kDataLoggerOpStartNoSettings (1 << 5)
#define kDataLoggerOpStartNoWiFi (1 << 6)

#define kDataLoggerOpStartAllFlags                                                                                     \
    (kDataLoggerOpStartNoAutoload | kDataLoggerOpStartListDevices | kDataLoggerOpStartNoSettings |                     \
     kDataLoggerOpStartNoWiFi)

#define inOpMode(__mode__) ((_opFlags & __mode__) == __mode__)
#define setOpMode(__mode__) _opFlags |= __mode__
#define clearOpMode(__mode__) _opFlags &= ~__mode__

// forward declare of our system info class
class sfeDLSystemOp;

/////////////////////////////////////////////////////////////////////////
// Define our application class for the data logger
/////////////////////////////////////////////////////////////////////////
class sfeDataLogger : public flxApplication
{
  private:
    //---------------------------------------------------------------------------
    // setupSDCard()
    //
    // Set's up the SD card subsystem and the objects/systems that use it.
    bool setupSDCard(void);

    //---------------------------------------------------------------------
    // Setup the IOT clients
    bool setupIoTClients(void);

    //---------------------------------------------------------------------
    // Setup time ...
    bool setupTime(void);

  public:
    //---------------------------------------------------------------------------
    // Constructor
    //

    sfeDataLogger();

    //---------------------------------------------------------------------------
    // onSetup()
    //
    // Called by the system before devices are loaded, and system initialized
    bool onSetup();

    //---------------------------------------------------------------------------
    // onDeviceLoad()
    //
    // Called by the system, right after device auto-load, but before system restore
    // Allows the app to load other devices.
    void onDeviceLoad(void);

    void onRestore(void);

    void resetDevice(void);

  private:
    friend class sfeDLCommands;
    friend class sfeDLSystemOp;

    //---------------------------------------------------------------------
    // Check if we have a NFC reader available -- for use with WiFi credentials
    //
    // Call after auto-load
    void setupNFDevice(void);

    //---------------------------------------------------------------------
    void setupBioHub(void);

    //---------------------------------------------------------------------
    void setupENS160(void);

    //------------------------------------------
    // For controlling the log output types

    static constexpr uint8_t kAppLogTypeNone = 0x0;
    static constexpr uint8_t kAppLogTypeCSV = 0x1;
    static constexpr uint8_t kAppLogTypeJSON = 0x2;

    static constexpr char *kLogFormatNames[] = {"Disabled", "CSV Format", "JSON Format"};

    // Startup output modes
    static constexpr uint8_t kAppStartupMsgNormal = 0x0;
    static constexpr uint8_t kAppStartupMsgCompact = 0x1;
    static constexpr uint8_t kAppStartupMsgNone = 0x2;

    //---------------------------------------------------------------------------
    uint8_t get_logTypeSD(void);

    //---------------------------------------------------------------------------
    void set_logTypeSD(uint8_t logType);

    //---------------------------------------------------------------------------
    uint8_t get_logTypeSer(void);

    //---------------------------------------------------------------------------
    void set_logTypeSer(uint8_t logType);

    void about_app_status(void)
    {
        displayAppAbout();
    }
    uint8_t _logTypeSD;
    uint8_t _logTypeSer;

    // For the terminal baud rate setting

    uint _terminalBaudRate;

    uint get_termBaudRate(void);
    void set_termBaudRate(uint rate);

    bool get_ledEnabled(void);
    void set_ledEnabled(bool);

    bool get_sleepEnabled(void);
    void set_sleepEnabled(bool);

    uint get_sleepWakePeriod(void);
    void set_sleepWakePeriod(uint);

    uint get_jsonBufferSize(void);
    void set_jsonBufferSize(uint);

    bool get_verbose_dev_name(void);
    void set_verbose_dev_name(bool);

    std::string get_local_name(void);
    void set_local_name(std::string name);

    // color text
    bool get_color_text(void);
    void set_color_text(bool);

    // for enabling system info in the log stream

    bool get_logsysinfo(void);
    void set_logsysinfo(bool);

    // support for onInit
    void onInitStartupCommands(uint);

  public:
    //---------------------------------------------------------------------------

    // onInit()
    //
    // Called before anything is started
    void onInit();

    // onStart()
    //
    // Called after the system is loaded, restored and initialized
    bool onStart();

    bool loop();

    // Define our log type properties

    flxPropertyRWUint8<sfeDataLogger, &sfeDataLogger::get_logTypeSD, &sfeDataLogger::set_logTypeSD> sdCardLogType = {
        kAppLogTypeCSV,
        {{kLogFormatNames[kAppLogTypeNone], kAppLogTypeNone},
         {kLogFormatNames[kAppLogTypeCSV], kAppLogTypeCSV},
         {kLogFormatNames[kAppLogTypeJSON], kAppLogTypeJSON}}};

    flxPropertyRWUint8<sfeDataLogger, &sfeDataLogger::get_logTypeSer, &sfeDataLogger::set_logTypeSer> serialLogType = {
        kAppLogTypeCSV,
        {{kLogFormatNames[kAppLogTypeNone], kAppLogTypeNone},
         {kLogFormatNames[kAppLogTypeCSV], kAppLogTypeCSV},
         {kLogFormatNames[kAppLogTypeJSON], kAppLogTypeJSON}}};

    // JSON output buffer size
    flxPropertyRWUint<sfeDataLogger, &sfeDataLogger::get_jsonBufferSize, &sfeDataLogger::set_jsonBufferSize>
        jsonBufferSize = {100, 5000};

    // System sleep properties
    flxPropertyUint<sfeDataLogger> sleepInterval = {5, 86400};
    flxPropertyRWUint<sfeDataLogger, &sfeDataLogger::get_sleepWakePeriod, &sfeDataLogger::set_sleepWakePeriod>
        wakeInterval = {60, 86400};
    flxPropertyRWBool<sfeDataLogger, &sfeDataLogger::get_sleepEnabled, &sfeDataLogger::set_sleepEnabled> sleepEnabled;

    // Display LED Enabled?
    flxPropertyRWBool<sfeDataLogger, &sfeDataLogger::get_ledEnabled, &sfeDataLogger::set_ledEnabled> ledEnabled;

    // Serial Baud rate setting
    flxPropertyRWUint<sfeDataLogger, &sfeDataLogger::get_termBaudRate, &sfeDataLogger::set_termBaudRate>
        serialBaudRate = {1200, 500000};

    // Verbose Device Names
    flxPropertyRWBool<sfeDataLogger, &sfeDataLogger::get_verbose_dev_name, &sfeDataLogger::set_verbose_dev_name>
        verboseDevNames;

    flxParameterInVoid<sfeDataLogger, &sfeDataLogger::about_app_status> aboutApplication;

    // board user set name
    flxPropertyRWString<sfeDataLogger, &sfeDataLogger::get_local_name, &sfeDataLogger::set_local_name> localBoardName;

    // Color Text Output
    flxPropertyRWBool<sfeDataLogger, &sfeDataLogger::get_color_text, &sfeDataLogger::set_color_text> colorTextOutput = {
        true};

    // startup delay setting
    flxPropertyUint<sfeDataLogger> startupDelaySecs = {0, 60};

    flxPropertyUint8<sfeDataLogger> startupOutputMode = {
        kAppStartupMsgNormal,
        {{"Normal", kAppStartupMsgNormal}, {"Compact", kAppStartupMsgCompact}, {"Disabled", kAppStartupMsgNone}}};

    // log system info
    // Enabled/Disabled
    flxPropertyRWBool<sfeDataLogger, &sfeDataLogger::get_logsysinfo, &sfeDataLogger::set_logsysinfo> logSysInfo;

  private:
    void enterSleepMode(void);
    void outputVMessage(void);
    void checkOpMode(void);

    void _displayAboutObjHelper(char, const char *, bool);
    void displayAppAbout(void);
    void displayAppStatus(bool useInfo = false);

    // event things
    void onFirmwareLoad(bool bLoading);
    void onSettingsEdit(bool bLoading);

    void onSystemActivity(void);
    void onSystemActivityLow(void);

    void onErrorMessage(uint8_t);

    void getStartupProperties(uint &baudRate, uint &startupDelay);

    // Board button callbacks
    void onButtonPressed(uint);
    void onButtonReleased(uint);

    // battery level checks
    void checkBatteryLevels(void);

    // Class members -- that make up the application structure

    // WiFi and NTP
    flxWiFiESP32 _wifiConnection;
    flxNTPESP32 _ntpClient;

    // Create a JSON and CSV output formatters.
    // Note: setting internal buffer sizes using template to minimize alloc calls.
    flxFormatJSON<kAppJSONDocSize> _fmtJSON;
    flxFormatCSV _fmtCSV;

    // Our logger
    flxLogger _logger;

    // Timer for event logging
    flxTimer _timer;

    // SD Card Filesystem object
    flxFSSDMMCard _theSDCard;

    // A writer interface for the SD Card that also rotates files
    flxFileRotate _theOutputFile;

    // settings things
    flxStorageESP32Pref _sysStorage;
    flxSettingsSerial _serialSettings;
    flxStorageJSONPrefFile _jsonStorage;

    // the onboard IMU
    flxDevISM330_SPI _onboardIMU;
    flxDevMMC5983_SPI _onboardMag;

    // a biometric sensor hub
    flxDevBioHub _bioHub;

    // IoT endpoints
    // An generic MQTT client
    flxMQTTESP32 _mqttClient;

    // secure mqtt
    flxMQTTESP32Secure _mqttSecureClient;

    // AWS
    flxIoTAWS _iotAWS;

    // Thingspeak
    flxIoTThingSpeak _iotThingSpeak;

    // azure
    flxIoTAzure _iotAzure;

    // HTTP/URL Post
    flxIoTHTTP _iotHTTP;

    // machine chat Iot
    flxIoTMachineChat _iotMachineChat;

    // Arduino IoT
    flxIoTArduino _iotArduinoIoT;

    // KDB Testing
    // Web Server
    sfeDLWebServer _iotWebServer;

    // Our firmware Update/Reset system
    flxSysFirmware _sysUpdate;

    // for our button events of the board
    sfeDLButton _boardButton;

    // For the sleep timer
    unsigned long _startTime = 0;

    bool _isValidMode;

    uint32_t _modeFlags;
    uint32_t _opFlags;

    // Fuel gauge
    flxDevMAX17048 *_fuelGauge;

    // // oled
    // flxDevMicroOLED *_microOLED;

    // battery check event
    std::unique_ptr<flxJob> _batteryJob;

    // sleep things - is enabled storage, sleep event
    bool _bSleepEnabled;
    flxJob _sleepJob;

    // log sys info

    bool _bLogSysInfo;
    sfeDLSystemOp *_pSystemInfo;

    // #ifdef ENABLE_OLED_DISPLAY
    //     sfeDLDisplay *_pDisplay;
    // #endif
};
