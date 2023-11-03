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
 * Spark Framework demo - logging
 *
 */

// Spark framework
#include <Flux.h>
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
static const uint8_t kAppOnBoardIMUCS = 5;

// The onboard Magnetometer
#include <Flux/flxDevMMC5983.h>
static const uint8_t kAppOnBoardMAGCS = 27;

// Biometric Hub -- requires pins to be set on startup
#include <Flux/flxDevBioHub.h>
static const uint8_t kAppBioHubReset = 17; // Use the TXD pin as the bio hub reset pin
static const uint8_t kAppBioHubMFIO = 16;  // Use the RXD pin as the bio hub mfio pin

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
#include <Flux/flxDevMicroOLED.h>

#include "sfeDLButton.h"

#ifdef ENABLE_OLED_DISPLAY
#include "sfeDLDisplay.h"
#endif

//------------------------------------------
// Default log interval in milli secs
#define kDefaultLogInterval 15000

// Buffersize of our JSON document output
#define kAppJSONDocSize 1600

// NTP Startup delay in secs

#define kAppNTPStartupDelaySecs 5

// Battery check interval (90 seconds)
#define kBatteryCheckInterval 90000

// Default Sleep Periods
#define kSystemSleepSleepSec 60
#define kSystemSleepWakeSec 120

//-----------------------------------------------------------------
// Helper class for loop events - unifies the idea of calling a method
// after a specific time ...
class sfeDLLoopEvent
{

  public:
    sfeDLLoopEvent(const char *label) : name{label}, delta{0}, last{0}
    {
    }

    sfeDLLoopEvent(const char *label, uint32_t in_delta, uint32_t in_last) : sfeDLLoopEvent(label)
    {
        delta = in_delta;
        last = in_last;
    }

    // what method to call when time expried
    template <typename T> void call(T *inst, void (T::*func)())
    {
        handler = [=]() { // uses a lambda for the callback
            (inst->*func)();
        };
    }

    // handler
    std::function<void()> handler;

    // Time delta in MS
    uint32_t delta;

    // Last time checked im MS;
    uint32_t last;

    // Event name = helpful
    const char *name;
};

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
    // Called by the system, right after device autoload, but before system restore
    // Allows the app to load other devices.
    void onDeviceLoad(void);

    void onRestore(void);

    void resetDevice(void);

  private:
    //---------------------------------------------------------------------
    // Check if we have a NFC reader available -- for use with WiFi credentials
    //
    // Call after autoload
    void setupNFDevice(void);

    //---------------------------------------------------------------------
    void setupBioHub(void);

    //------------------------------------------
    // For controlling the log output types

    static constexpr uint8_t kAppLogTypeNone = 0x0;
    static constexpr uint8_t kAppLogTypeCSV = 0x1;
    static constexpr uint8_t kAppLogTypeJSON = 0x2;

    static constexpr char *kLogFormatNames[] = {"Disabled", "CSV Format", "JSON Format"};

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
    flxPropertyRWUint<sfeDataLogger, &sfeDataLogger::get_jsonBufferSize, &sfeDataLogger::set_jsonBufferSize> jsonBuferSize = {100, 5000};

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

    flxParameterInVoid<sfeDataLogger, &sfeDataLogger::about_app_status> aboutApplication;


  private:
    void enterSleepMode(void);
    void outputVMessage(void);
    void checkOpMode(void);

    void displayAppAbout(void);
    void displayAppStatus(bool useInfo = false);

    // event things
    void onFirmwareLoad(bool bLoading);
    void listenForFirmwareLoad(flxSignalBool &theEvent);

    void onSettingsEdit(bool bLoading);
    void listenForSettingsEdit(flxSignalBool &theEvent);

    uint getTerminalBaudRate(void);

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
    flxStorageJSONPref _jsonStorage;

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

    // Our firmware Update/Reset system
    flxSysFirmware _sysUpdate;

    // for our button events of the board
    sfeDLButton _boardButton;

    // For the sleep timer
    unsigned long _startTime = 0;

    bool _isValidMode;

    uint32_t _modeFlags;
    uint16_t _opFlags;

    // Fuel gauge
    flxDevMAX17048 *_fuelGauge;

    // oled
    flxDevMicroOLED *_microOLED;

    // loop event things
    std::vector<sfeDLLoopEvent *> _loopEventList;

    // battery check event
    sfeDLLoopEvent _batteryEvent = {"BATTERY", kBatteryCheckInterval, 0};

    // sleep things - is enabled storage, sleep event
    bool _bSleepEnabled;
    sfeDLLoopEvent _sleepEvent = {"SLEEP", kSystemSleepWakeSec * 1000, 0};

#ifdef ENABLE_OLED_DISPLAY
    sfeDLDisplay *_pDisplay;
#endif
};
