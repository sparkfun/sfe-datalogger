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

// IoT Client Includes
#include <Flux/flxIoTAWS.h>
#include <Flux/flxIoTAzure.h>
#include <Flux/flxIoTHTTP.h>
#include <Flux/flxIoTMachineChat.h>
#include <Flux/flxIoTThingSpeak.h>
#include <Flux/flxMQTTESP32.h>

// System Firmware update/reset
#include <Flux/flxSysFirmware.h>
//------------------------------------------
// Default log interval in milli secs
#define kDefaultLogInterval 15000

// Buffersize of our JSON document output
#define kAppJSONDocSize 1600

// NTP Startup delay in secs

#define kAppNTPStartupDelaySecs 5

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
    // setup()
    //
    // Called by the system before devices are loaded, and system initialized
    bool setup();

    //---------------------------------------------------------------------------
    // onDeviceLoad()
    //
    // Called by the system, right after device autoload, but before system restore
    // Allows the app to load other devices.
    void onDeviceLoad(void);

    void onRestore(void);

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

  public:
    //---------------------------------------------------------------------------

    // init()
    // 
    // Called before anything is started 
    void init();

    // start()
    //
    // Called after the system is loaded, restored and initialized
    bool start();

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

    // System sleep properties
    flxPropertyInt<sfeDataLogger> sleepInterval = {5, 86400};
    flxPropertyInt<sfeDataLogger> wakeInterval = {60, 86400};
    flxPropertyBool<sfeDataLogger> sleepEnabled = {false};

    // Display LED Enabled?
    flxPropertyBool<sfeDataLogger> ledEnabled = {true};

    // Serial Baud rate setting
    flxPropertyUint<sfeDataLogger> serialBaudRate = {1200, 500000};

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

    // Our firmware Update/Reset system
    flxSysFirmware _sysUpdate;

    // For the sleep timer
    unsigned long _startTime = 0;

    bool _isValidMode;

    unsigned long _lastLCheck;
    uint16_t _modeFlags;
};
