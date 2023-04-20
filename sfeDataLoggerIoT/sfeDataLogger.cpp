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
 * SparkFun Data Logger
 *
 */

#include "sfeDataLogger.h"
#include "dl_version.h"
#include "dl_mode.h"

#include "esp_sleep.h"

// for our time setup
#include <Flux/flxClock.h>
#include <Flux/flxDevGNSS.h>
#include <Flux/flxDevMAX17048.h>
#include <Flux/flxDevRV8803.h>

RTC_DATA_ATTR int boot_count = 0;

// For finding the firmware files on SD card
#define kDataLoggerFirmwareFilePrefix "SparkFunDataLoggerIoT"

// Application keys - used to encrypt runtime secrets for the app.
//
// NOTE: Gen a base 64 key  % openssl rand -base64 32
//       Convert into ascii ints in python %    data = [ord(c) for c in ss]
//       Jam into the below array

// If a key array is passed in via a #define, use that, otherwise use a default, dev key
#ifdef DATALOGGER_IOT_APP_KEY
static uint8_t _app_jump[] = DATALOGGER_IOT_APP_KEY;
#else
static uint8_t _app_jump[] = {104, 72, 67, 51,  74,  67,  108, 99, 104, 112, 77,  100, 55,  106, 56,
                              78,  68, 69, 108, 98,  118, 51,  65, 90,  48,  51,  82,  111, 120, 56,
                              52,  49, 70, 76,  103, 77,  84,  49, 85,  99,  117, 66,  111, 61};
#endif

// The datalogger firmware OTA manifest  URL

#define kDataLoggerOTAManifestURL                                                                                      \
    "https://raw.githubusercontent.com/gigapod/ota-demo-exp/main/manifiest/sfe-dl-manifest.json"

// Valid platform check interface

#ifdef DATALOGGER_IOT_NAG_TIME
#define kLNagTimeMins DATALOGGER_IOT_NAG_TIME
#else
#define kLNagTimeMins 30
#endif

#define kLNagTimeSecs (kLNagTimeMins * 60)

#define kLNagTimesMSecs (kLNagTimeSecs * 1000)

static char *kLNagMessage =
    "This firmware is designed to run on a SparkFun DataLogger IoT board. Purchase one at www.sparkfun.com";



constexpr char *sfeDataLogger::kLogFormatNames[];
//---------------------------------------------------------------------------
// Constructor
//

sfeDataLogger::sfeDataLogger()
    : _logTypeSD{kAppLogTypeNone}, _logTypeSer{kAppLogTypeNone}, _timer{kDefaultLogInterval}, _isValidMode{false},
      _lastLCheck{0}, _modeFlags{0}
{

    // Add a title for this section - the application level  - of settings
    setTitle("General");

    flxRegister(ledEnabled, "LED Enabled", "Enable/Disable the on-board LED activity");

    sdCardLogType.setTitle("Output Formats");
    flxRegister(sdCardLogType, "SD Card Format", "Enable and set the output format");
    flxRegister(serialLogType, "Serial Console Format", "Enable and set the output format");

    // Add the format changing props to the logger - makes more sense from a UX standpoint.
    _logger.addProperty(sdCardLogType);
    _logger.addProperty(serialLogType);

    _logger.setTitle("Logging");

    // sleep properties
    sleepEnabled.setTitle("Sleep");
    flxRegister(sleepEnabled, "Enable System Sleep", "If enabled, sleep the system ");
    flxRegister(sleepInterval, "Sleep Interval (sec)", "The interval the system will sleep for");
    flxRegister(wakeInterval, "Wake Interval (sec)", "The interval the system will operate between sleep period");

    // about?
    flxRegister(aboutApplication, "About...", "Details about the system");
    aboutApplication.prompt = false; // no prompt needed before execution

    // Update timer object string
    _timer.setName("Logging Timer", "Set the internal between log entries");

    // set some simple defaults
    sleepInterval = 30;
    wakeInterval = 120;

    flux.setAppToken(_app_jump, sizeof(_app_jump));
}

//---------------------------------------------------------------------------
// setupSDCard()
//
// Set's up the SD card subsystem and the objects/systems that use it.
bool sfeDataLogger::setupSDCard(void)
{
    // setup output to the SD card
    if (_theSDCard.initialize())
    {

        _theOutputFile.setName("Data File", "Output file rotation manager");

        // SD card is available - lets setup output for it
        // Add the filesystem to the file output/rotation object
        _theOutputFile.setFileSystem(_theSDCard);

        // setup our file rotation parameters
        _theOutputFile.filePrefix = "sfe";
        _theOutputFile.startNumber = 1;
        _theOutputFile.rotatePeriod(24); // one day

        // add the file output to the CSV output.
        //_fmtCSV.add(_theOutputFile);

        // have the CSV formatter listen to the new file event. This
        // will cause a header to be written next cycle.
        _fmtCSV.listenNewFile(_theOutputFile.on_newFile);

        return true;
    }
    return false;
}

//---------------------------------------------------------------------
// Setup the IOT clients
bool sfeDataLogger::setupIoTClients()
{

    // Add title for this section
    _mqttClient.setTitle("IoT Services");
    // setup the network connection for the mqtt
    _mqttClient.setNetwork(&_wifiConnection);
    // add mqtt to JSON
    _fmtJSON.add(_mqttClient);

    // setup the network connection for the mqtt
    _mqttSecureClient.setNetwork(&_wifiConnection);
    // add mqtt to JSON
    _fmtJSON.add(_mqttSecureClient);

    // AWS
    _iotAWS.setName("AWS IoT", "Connect to an AWS Iot Thing");
    _iotAWS.setNetwork(&_wifiConnection);

    // Add the filesystem to load certs/keys from the SD card
    _iotAWS.setFileSystem(&_theSDCard);
    _fmtJSON.add(_iotAWS);

    // Thingspeak driver
    _iotThingSpeak.setNetwork(&_wifiConnection);

    // Add the filesystem to load certs/keys from the SD card
    _iotThingSpeak.setFileSystem(&_theSDCard);
    _fmtJSON.add(_iotThingSpeak);

    // Azure IoT
    _iotAzure.setNetwork(&_wifiConnection);

    // Add the filesystem to load certs/keys from the SD card
    _iotAzure.setFileSystem(&_theSDCard);
    _fmtJSON.add(_iotAzure);

    // general HTTP / URL logger
    _iotHTTP.setNetwork(&_wifiConnection);
    _iotHTTP.setFileSystem(&_theSDCard);
    _fmtJSON.add(_iotHTTP);


     // Machine Chat 
    _iotMachineChat.setNetwork(&_wifiConnection);
    _iotMachineChat.setFileSystem(&_theSDCard);
    _fmtJSON.add(_iotMachineChat);

    return true;
}

//---------------------------------------------------------------------------
// setupTime()
//
// Setup any time sources/sinks. Called after devices are loaded

bool sfeDataLogger::setupTime()
{

    // what is our clock - as setup from init/prefs
    std::string refClock = flxClock.referenceClock();

    flxClock.addReferenceClock(&_ntpClient, _ntpClient.name());

    // Any GNSS devices attached?
    auto allGNSS = flux.get<flxDevGNSS>();
    for (auto gnss : *allGNSS)
        flxClock.addReferenceClock(gnss, gnss->name());

    // RTC clock?
    auto allRTC8803 = flux.get<flxDevRV8803>();
    for (auto rtc8803 : *allRTC8803)
    {
        flxClock.addReferenceClock(rtc8803, rtc8803->name());
        flxClock.addConnectedClock(rtc8803);
    }

    // Now that clocks are loaded, set the ref clock to what was started with. 
    flxClock.referenceClock = refClock;

    // update the system clock to the reference clock
    flxClock.updateClock();

    return true;
}

//---------------------------------------------------------------------------
// output things
//---------------------------------------------------------------------------

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

    time_t t_now;
    time(&t_now);
    struct tm *tmLocal = localtime(&t_now);

    char szBuffer[64];
    memset(szBuffer, '\0', sizeof(szBuffer));
    strftime(szBuffer, sizeof(szBuffer), "%G-%m-%dT%T", tmLocal);
    flxLog__(logLevel, "%cTime:\t%s", pre_ch, szBuffer);

    // uptime
    uint32_t days, hours, minutes, secs, mills;

    flx_utils::uptime(days, hours, minutes, secs, mills);

    flxLog__(logLevel, "%cUptime:\t%u days, %02u:%02u:%02u.%u", pre_ch, days, hours, minutes, secs, mills);
    flxLog__(logLevel, "%cExternal Time Source: %s", pre_ch, flxClock.referenceClock().c_str());

    if (!useInfo)
        flxLog_N("");

    flxLog__(logLevel, "%cBoard Name: %s", pre_ch, dlModeCheckName(_modeFlags));
    flxLog__(logLevel, "%cBoard ID: %s", pre_ch, flux.deviceId());

    if (!useInfo)
        flxLog_N("");

    if (_theSDCard.enabled())
        flxLog__(logLevel, "%cSD card - Type: %s, Size: %uMB, Used: %uMB", pre_ch, _theSDCard.type(), _theSDCard.size(),
                 _theSDCard.used());
    else
        flxLog__(logLevel, "%cSD card not available", pre_ch);

    if (_wifiConnection.enabled())
    {
        flxLog__(logLevel, "%cWiFi - SSID: %s, Connected: %s", pre_ch,
                 _wifiConnection.SSID().length() > 1 ? _wifiConnection.SSID().c_str() : "",
                 _wifiConnection.isConnected() ? "yes" : "no");
    }
    else
        flxLog__(logLevel, "%cWiFi not enabled", pre_ch);

    flxLog__(logLevel, "%cSystem Deep Sleep: %s", pre_ch, sleepEnabled() ? "enabled" : "disabled");
    flxLog_N("%c    Sleep Interval:  %d seconds", pre_ch, sleepInterval());
    flxLog_N("%c    Wake Interval:   %d seconds", pre_ch, wakeInterval());    

    flxLog_N("");

    flxLog__(logLevel, "%cLogging Interval (ms): %u", pre_ch, _timer.interval());
    flxLog__(logLevel, "%cSerial Output:  %s", pre_ch, kLogFormatNames[serialLogType()]);
    flxLog__(logLevel, "%cSD Card Output: %s", pre_ch, kLogFormatNames[sdCardLogType()]);
    flxLog_N("%c    Current Filename: \t%s", pre_ch,  
        _theOutputFile.currentFilename().length() == 0 ? "``" : _theOutputFile.currentFilename().c_str());
    flxLog_N("%c    Rotate Period: \t%d Hours", pre_ch, _theOutputFile.rotatePeriod());

    flxLog_N("");

    flxLog__(logLevel, "%cIoT Services:", pre_ch);

    flxLog_N("%c    %s  \t: %s", pre_ch, _mqttClient.name(), _mqttClient.enabled() ? "enabled" : "disabled");
    flxLog_N("%c    %s  : %s", pre_ch, _mqttSecureClient.name(), _mqttSecureClient.enabled() ? "enabled" : "disabled");    
    flxLog_N("%c    %s  \t\t: %s", pre_ch, _iotHTTP.name(), _iotHTTP.enabled() ? "enabled" : "disabled");
    flxLog_N("%c    %s  \t\t: %s", pre_ch, _iotAWS.name(), _iotAWS.enabled() ? "enabled" : "disabled");
    flxLog_N("%c    %s  \t\t: %s", pre_ch, _iotAzure.name(), _iotAzure.enabled() ? "enabled" : "disabled");
    flxLog_N("%c    %s  \t: %s", pre_ch, _iotThingSpeak.name(), _iotThingSpeak.enabled() ? "enabled" : "disabled");
    flxLog_N("%c    %s  \t: %s", pre_ch, _iotMachineChat.name(), _iotMachineChat.enabled() ? "enabled" : "disabled");    

    flxLog_N("");

    // connected devices...
    flxDeviceContainer myDevices = flux.connectedDevices();
    flxLog__(logLevel, "%cConnected Devices [%d]:", pre_ch, myDevices.size());

    // Loop over the device list - note that it is iterable.
    for (auto device : myDevices)
        flxLog_N("%c    %s \t- %s  {%s}", pre_ch, device->name(), device->description(),
                device->getKind() == flxDeviceKindI2C ? "qwiic" : "SPI");

    flxLog_N("");
}

//---------------------------------------------------------------------------
void sfeDataLogger::displayAppAbout()
{

    char szBuffer[128];
    flux.versionString(szBuffer, sizeof(szBuffer), true);

    flxLog_N("\n\r\t%s   %s", flux.name(), flux.description());
    flxLog_N("\tVersion: %s\n\r", szBuffer);

    displayAppStatus(false);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// setup()
//
// Called by the system before devices are loaded, and system initialized
bool sfeDataLogger::setup()
{
    // Lets set the application name?!
    setName("SparkFun DataLogger IoT - 9DOF", "(c) 2023 SparkFun Electronics");

    // Version info
    setVersion(kDLVersionNumberMajor, kDLVersionNumberMinor, kDLVersionNumberPoint, kDLVersionDescriptor, BUILD_NUMBER);
    setAppClassID(kDLAppClassNameID); // internal name string for this app type

    // set the settings storage system for spark
    flxSettings.setStorage(&_sysStorage);
    flxSettings.setFallback(&_jsonStorage);

    // Have JSON storage write/use the SD card
    _jsonStorage.setFileSystem(&_theSDCard);
    _jsonStorage.setFilename("datalogger.json");

    // Have settings saved when editing via serial console is complete.
    flxSettings.listenForSave(_serialSettings.on_finished);

    // Add serial settings to spark - the spark loop call will take care
    // of everything else.
    flux.add(_serialSettings);

    _wifiConnection.setTitle("Network");

    // wire up the NTP to the wifi network object. When the connection status changes,
    // the NTP client will start and stop.
    _ntpClient.setNetwork(&_wifiConnection);
    _ntpClient.setStartupDelay(kAppNTPStartupDelaySecs); // Give the NTP server some time to start

    // set our default clock to NTP - this will be overwritten if prefs are loaded
    flxClock.referenceClock = _ntpClient.name();

    // setup SD card. Do this before calling start - so prefs can be read off SD if needed
    if (!setupSDCard())
    {
        flxLog_W(F("Error initializing the SD Card. Is an SD card installed on the board?"));

        // disable SD card output
        set_logTypeSD(kAppLogTypeNone);
    }

    // Setup the IoT clients
    if (!setupIoTClients())
        flxLog_W(F("Error initializing IoT Clients"));

    //----------
    // setup firmware update/reset system

    // Filesystem to read firmware from
    _sysUpdate.setFileSystem(&_theSDCard);

    // Serial UX - used to list files to select off the filesystem
    _sysUpdate.setSerialSettings(_serialSettings);

    _sysUpdate.setFirmwareFilePrefix(kDataLoggerFirmwareFilePrefix);

    _sysUpdate.setWiFiDevice(&_wifiConnection);
    _sysUpdate.enableOTAUpdates(kDataLoggerOTAManifestURL);

    // Add to the system - manual add so it appears last in the ops list

    _sysUpdate.setTitle("Advanced");
    flux.add(_sysUpdate);

    return true;
}

//---------------------------------------------------------------------
// Check if we have a NFC reader available -- for use with WiFi credentials
//
// Call after autoload
void sfeDataLogger::setupNFDevice(void)
{
    // do we have a NFC device connected?
    auto nfcDevices = flux.get<flxDevST25DV>();

    if (nfcDevices->size() == 0)
        return;

    // We have an NFC device. Create a credentials action and connect to the NFC device
    // and WiFi.
    flxSetWifiCredentials *pCreds = new flxSetWifiCredentials;

    if (!pCreds)
        return;

    flxDevST25DV *pNFC = nfcDevices->at(0);
    flxLog_I(F("%s: WiFi credentials via NFC enabled"), pNFC->name());

    pCreds->setCredentialSource(pNFC);
    pCreds->setWiFiDevice(&_wifiConnection);

    // Change the name on the action
    pCreds->setName("WiFi Login From NFC", "Set the devices WiFi Credentials from an attached NFC source.");
}

//---------------------------------------------------------------------
// onDeviceLoad()
//
// Called after qwiic/i2c autoload, but before system state restore

void sfeDataLogger::onDeviceLoad()
{
    // Load SPI devices
    // Note - framework is setting up the pins ...

    // IMU
    if (_onboardIMU.initialize(kAppOnBoardIMUCS))
        _modeFlags |= DL_MODE_FLAG_IMU;

    // Magnetometer
    if (_onboardMag.initialize(kAppOnBoardMAGCS))
        _modeFlags |= DL_MODE_FLAG_MAG;

    // quick check on fuel gauge - which is part of the IOT 9DOF board
    auto fuelGuage = flux.get<flxDevMAX17048>();

    if (fuelGuage->size() > 0)
        _modeFlags |= DL_MODE_FLAG_FUEL;
}

//---------------------------------------------------------------------
void sfeDataLogger::setupBioHub()
{
    if (_bioHub.initialize(kAppBioHubReset,
                           kAppBioHubMFIO)) // Initialize the bio hub using the reset and mfio pins,
    {
        flxLog_I(F("%s is enabled"), _bioHub.name());
        _logger.add(_bioHub);
    }
}

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
// Check our platform status
void sfeDataLogger::checkOpMode()
{
    _isValidMode = dlModeCheckValid(_modeFlags);
}
//---------------------------------------------------------------------------
// start()
//
// Called after the system is loaded, restored and initialized
bool sfeDataLogger::start()
{

    // Waking up from a sleep (boot count isn't zero)
    if (boot_count != 0)
    {
        flxLog_I(F("Starting system from deep sleep - boot_count %d - wake period is %d seconds"), boot_count,
                 wakeInterval());

        // Print the wakeup reason
        esp_sleep_wakeup_cause_t wakeup_reason;
        wakeup_reason = esp_sleep_get_wakeup_cause();
        switch (wakeup_reason)
        {
        case ESP_SLEEP_WAKEUP_EXT0:
            flxLog_I(F("Wakeup caused by external signal using RTC_IO"));
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            flxLog_I(F("Wakeup caused by external signal using RTC_CNTL"));
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            flxLog_I(F("Wakeup caused by timer"));
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            flxLog_I(F("Wakeup caused by touchpad"));
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            flxLog_I(F("Wakeup caused by ULP program"));
            break;
        default:
            flxLog_I(F("Wakeup was not caused by deep sleep: %d"), (int)wakeup_reason);
            break;
        }
    }

    boot_count++;

    // Logging is done at an interval - using an interval timer.
    // Connect logger to the timer event
    _logger.listen(_timer.on_interval);

    //  - Add the JSON and CVS format to the logger
    _logger.add(_fmtJSON);
    _logger.add(_fmtCSV);

    // setup NFC - it provides another means to load WiFi credentials
    setupNFDevice();

    // check our I2C devices
    // Loop over the device list - note that it is iterable.
    flxLog_I_(F("Loading devices ... "));
    flxDeviceContainer loadedDevices = flux.connectedDevices();

    if (loadedDevices.size() == 0)
        flxLog_N(F("no devices detected"));
    else
    {
        flxLog_N(F("%d devices detected"), loadedDevices.size());

        for (auto device : loadedDevices)
        {
            flxLog_N(F("    %s\t\t- %s  {%s}"), device->name(), device->description(),
                    device->getKind() == flxDeviceKindI2C ? "qwiic" : "SPI");
            if (device->nOutputParameters() > 0)
                _logger.add(device);
        }
    }

    // Setup the Bio Hub
    setupBioHub();

    // Check time devices
    if (!setupTime())
        flxLog_W("Time reference setup failed.");

    flxLog_N("");

    // set our system start time im millis
    _startTime = millis();

    checkOpMode();

    displayAppStatus(true);

    if (!_isValidMode)
        outputVMessage();

    return true;
}

//---------------------------------------------------------------------------
void sfeDataLogger::enterSleepMode()
{

    if (!sleepEnabled())
        return;

    flxLog_I(F("Starting device deep sleep for %d secs"), sleepInterval());

    // esp_sleep_config_gpio_isolate(); // Don't. This causes: E (33643) gpio: gpio_sleep_set_pull_mode(827): GPIO
    // number error

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF); // Don't disable RTC SLOW MEM - otherwise
    // boot_count (RTC_DATA_ATTR) becomes garbage
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    unsigned long long period = sleepInterval() * 1000000ULL;

    esp_sleep_enable_timer_wakeup(period);

    esp_deep_sleep_start(); // see you on the other side
}
//---------------------------------------------------------------------------
void sfeDataLogger::outputVMessage()
{
    _logger.logMessage("INVALID PLATFORM", kLNagMessage);

    // if not logging to the serial console, dump out a message

    if (_logTypeSer == kAppLogTypeNone)
        flxLog_W(kLNagMessage);
}
//---------------------------------------------------------------------------
// loop()
//
// Called during the operational loop of the system.

bool sfeDataLogger::loop()
{
    // Is sleep enabled and if so, is it time to sleep the system
    if (sleepEnabled() && millis() - _startTime > wakeInterval() * 1000)
    {
        enterSleepMode();
    }

    // If this isn't a valid hardware platform, output a nag string
    if (!_isValidMode && millis() - _lastLCheck > kLNagTimesMSecs)
    {
        outputVMessage();
        _lastLCheck = millis();
    }

    return false;
}
