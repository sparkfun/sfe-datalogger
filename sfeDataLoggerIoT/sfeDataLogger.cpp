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

#include "esp_sleep.h"

// for our time setup
#include <Flux/flxClock.h>
#include <Flux/flxDevGNSS.h>
#include <Flux/flxDevRV8803.h>
#include <Flux/flxDevMAX17048.h>

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

#define kDataLoggerOTAManifestURL "https://raw.githubusercontent.com/gigapod/ota-demo-exp/main/manifiest/sfe-dl-manifest.json"

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

// devices - on board - flags
#define DL_MODE_FLAG_IMU    (1 << 0)
#define DL_MODE_FLAG_MAG    (1 << 1)
#define DL_MODE_FLAG_FUEL   (1 << 2)

#define SFE_DL_IOT_9DOF_MODE  (DL_MODE_FLAG_IMU | DL_MODE_FLAG_MAG | DL_MODE_FLAG_FUEL)

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
    flxRegister(sleepInterval, "Sleep Interval (S)", "The interval the system will sleep for");
    flxRegister(wakeInterval, "Wake Interval (S)", "The interval the system will operate between sleep period");


    // about?
    flxRegister(aboutApplication, "About...", "Details about the system");
    aboutApplication.prompt=false; // no prompt needed before execution

    // Update timer object string
    _timer.setName("Logging Timer", "Set the internal between log entries");

    // set some simple defaults
    sleepInterval = 20;
    wakeInterval = 30;

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

    return true;
}

//---------------------------------------------------------------------------
// setupTime()
//
// Setup any time sources/sinks. Called after devices are loaded

bool sfeDataLogger::setupTime()
{

    // Add NTP and set as Prime!
    if (!flxClock.setReferenceClock(&_ntpClient, _ntpClient.name()))
        flxLog_W(F("Unable to set %s to the reference clock during setup"), _ntpClient.name());

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

    // update connected clocks on a clock update
    flxClock.updateConnectedOnUpdate = true;

    // update the system clock to the reference clock
    flxClock.updateClock();

    return true;
}

void sfeDataLogger::showAppStatus(void)
{

    char szBuffer[128];
    flux.versionString(szBuffer, sizeof(szBuffer), true);

    flxLog_N("\n\r\t%s   %s", flux.name(), szBuffer);
    flxLog_N("\t%s\n\r", flux.description());

    Serial.printf("\tDevice ID: %s\n\r", flux.deviceId());

    time_t t_now;
    time(&t_now);
    struct tm *tmLocal = localtime(&t_now);

    memset(szBuffer, '\0', sizeof(szBuffer));
    strftime(szBuffer, sizeof(szBuffer), "%G-%m-%dT%T", tmLocal);
    Serial.printf("\tTime: %s\n\r", szBuffer);

    // uptime
    uint32_t days, hours, minutes, secs, mills;

    flx_utils::uptime(days, hours, minutes, secs, mills);

    Serial.printf("\tUptime: %u days, %02u:%02u:%02u.%u\n\r", days, hours, minutes, secs, mills);

    if (_theSDCard.enabled())
        Serial.printf("\tSD card available. Type: %s, Size: %uMB, Used: %uMB\n\r", 
                _theSDCard.type(), _theSDCard.size(), _theSDCard.used());
    else
        Serial.printf("SD card not available.\n\r");

    if (_wifiConnection.isConnected())
        Serial.printf("\tWiFi Network: %s\n\r", _wifiConnection.SSID().c_str() );
    else
        Serial.printf("\tWiFi not connected.\n\r");

    Serial.printf("\n\r");

}
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

    // setup SD card. Do this before calling start - so prefs can be read off SD if needed
    if (!setupSDCard())
        flxLog_W(F("Error initializing the SD Card"));

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
void sfeDataLogger::setupSPIDevices()
{
    // Note - framework is setting up the pins ...
    // IMU
    if (_onboardIMU.initialize(kAppOnBoardIMUCS))
    {
        flxLog_I(F("Onboard %s is enabled"), _onboardIMU.name());
        _logger.add(_onboardIMU);
        _modeFlags |= DL_MODE_FLAG_IMU;
    }
    else
        flxLog_E(F("Onboard %s failed to start"), _onboardIMU.name());

    // Magnetometer
    if (_onboardMag.initialize(kAppOnBoardMAGCS))
    {
        flxLog_I(F("Onboard %s is enabled"), _onboardMag.name());
        _logger.add(_onboardMag);
        _modeFlags |= DL_MODE_FLAG_MAG;
    }
    else
        flxLog_E(F("Onboard %s failed to start"), _onboardMag.name());


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

    _isValidMode = false;
    // Is this is a Data Logger 9DOF (2023)

    if ((_modeFlags & SFE_DL_IOT_9DOF_MODE) == SFE_DL_IOT_9DOF_MODE)
        _isValidMode = true;
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

    // printout the device ID
    flxLog_I(F("Device ID: %s"), flux.deviceId());

    // Write out the SD card stats
    if (_theSDCard.enabled())
        flxLog_I(F("SD card available. Type: %s, Size: %uMB, Used: %uMB"), 
                _theSDCard.type(), _theSDCard.size(), _theSDCard.used());
    else
        flxLog_W(F("SD card not available."));

    // WiFi status
    if (!_wifiConnection.isConnected())
        flxLog_E(F("Unable to connect to WiFi!"));

    // Logging is done at an interval - using an interval timer.
    // Connect logger to the timer event
    _logger.listen(_timer.on_interval);

    //  - Add the JSON and CVS format to the logger
    _logger.add(_fmtJSON);
    _logger.add(_fmtCSV);

    // setup NFC - it provides another means to load WiFi credentials
    setupNFDevice();

    // What devices has the system detected?
    // List them and add them to the logger

    flxDeviceContainer myDevices = flux.connectedDevices();

    // The device list can be added directly to the logger object using an
    // add() method call. This will only add devices with output parameters.
    //
    // Example:
    //      logger.add(myDevices);
    //
    // But for this app, let's loop over our devices and show how use the
    // device parameters.

    flxLog_I(F("Devices Detected [%d]"), myDevices.size());

    // Loop over the device list - note that it is iterable.
    for (auto device : myDevices)
    {
        flxLog_I(F("\tDevice: %s, Output Number: %d"), device->name(), device->nOutputParameters());
        if (device->nOutputParameters() > 0)
            _logger.add(device);
    }

    // Setup the Onboard IMU
    setupSPIDevices();

    // Setup the Bio Hub
    setupBioHub();

    // Check time devices
    if (!setupTime())
        flxLog_W("Time reference setup failed.");

    flxLog_N("");

    // set our system start time im millis
    _startTime = millis();

    checkOpMode();
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
