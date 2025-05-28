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
 * SparkFun Data Logger
 *
 */

#include "sfeDataLogger.h"
#include "sfeDLCommands.h"
#include "sfeDLLed.h"
#include "sfeDLMode.h"
#include "sfeDLSystemOp.h"
#include "sfeDLVersion.h"

#include "esp_sleep.h"

// for our time setup
#include <Flux/flxClock.h>
#include <Flux/flxDevButton.h>
#include <Flux/flxDevMAX17048.h>
#include <Flux/flxDevTwist.h>
#include <Flux/flxUtils.h>

#include <time.h>
// SPI Devices
// The onboard IMU
static const uint8_t kAppOnBoardIMUCS = 5;

// The onboard Magnetometer
static const uint8_t kAppOnBoardMAGCS = 27;

RTC_DATA_ATTR int boot_count = 0;

// For finding the firmware files on SD card
#define kDataLoggerFirmwareFilePrefix "SparkFun_DataLoggerIoT_"

// Application keys - used to encrypt runtime secrets for the app.
//
// NOTE: Gen a base 64 key  % openssl rand -base64 32
//       Convert into ASCII ints in python %    data = [ord(c) for c in ss]
//       Jam into the below array

// If a key array is passed in via a #define, use that, otherwise use a default, dev key
#ifdef DATALOGGER_IOT_APP_KEY
static const uint8_t _app_jump[] = DATALOGGER_IOT_APP_KEY;
#else
static const uint8_t _app_jump[] = {104, 72, 67, 51,  74,  67,  108, 99, 104, 112, 77,  100, 55,  106, 56,
                                    78,  68, 69, 108, 98,  118, 51,  65, 90,  48,  51,  82,  111, 120, 56,
                                    52,  49, 70, 76,  103, 77,  84,  49, 85,  99,  117, 66,  111, 61};
#endif

// The datalogger firmware OTA manifest  URL
// Testing repo location
// #define kDataLoggerOTAManifestURL
// "https://raw.githubusercontent.com/gigapod/ota-demo-exp/main/manifiest/sfe-dl-manifest.json"

// Final/Deploy repo
#define kDataLoggerOTAManifestURL                                                                                      \
    "https://raw.githubusercontent.com/sparkfun/SparkFun_DataLogger/main/firmware/manifest/sfe-dl-manifest.json"

// Button event increment
#define kButtonPressedIncrement 5

// Startup/Timeout for serial connection to init...
#define kSerialStartupDelayMS 5000

//---------------------------------------------------------
// Valid platform check interface

#ifdef DATALOGGER_IOT_NAG_TIME
#define kLNagTimeMins DATALOGGER_IOT_NAG_TIME
#else
#define kLNagTimeMins 30
#endif

#define kLNagTimeSecs (kLNagTimeMins * 60)
#define kLNagTimesMSecs (kLNagTimeSecs * 1000)

static const char *kLNagMessage =
    "This firmware is designed to run on a SparkFun DataLogger IoT board. Purchase one at www.sparkfun.com";

#define kBatteryNoBatterySOC 110.

constexpr char *sfeDataLogger::kLogFormatNames[];

// delay used in loop during startup
const uint32_t kStartupLoopDelayMS = 70;

//---------------------------------------------------------------------------
// Constructor
//---------------------------------------------------------------------------
//

sfeDataLogger::sfeDataLogger()
    : _logTypeSD{kAppLogTypeNone}, _logTypeSer{kAppLogTypeNone}, _timer{kDefaultLogInterval}, _isValidMode{false},
      _modeFlags{0}, _opFlags{0}, _fuelGauge{nullptr}, _bSleepEnabled{false}, _bLogSysInfo{false}, _pSystemInfo{nullptr}
#ifdef ENABLE_OLED_DISPLAY
      ,
      _pDisplay{nullptr}
#endif
{

    // Add a title for this section - the application level  - of settings
    setTitle("General");

    flxRegister(ledEnabled, "LED Enabled", "Enable/Disable the on-board LED activity");

    // our the menu timeout property to our props/menu system entries
    addProperty(_serialSettings.menuTimeout);
    flxRegister(colorTextOutput, "Color Output", "Use color output with the Serial console");

    // user defined board name
    flxRegister(localBoardName, "Board Name", "A specific name for this DataLogger");

    sdCardLogType.setTitle("Output");
    flxRegister(sdCardLogType, "SD Card Format", "Enable and set the output format");
    flxRegister(serialLogType, "Serial Console Format", "Enable and set the output format");
    flxRegister(jsonBufferSize, "JSON Buffer Size", "Output buffer size in bytes");

    // Terminal Serial Baud Rate
    flxRegister(serialBaudRate, "Terminal Baud Rate", "Update terminal baud rate. Changes take effect on restart");
    _terminalBaudRate = kDefaultTerminalBaudRate;

    // Add the format changing props to the logger - makes more sense from a UX standpoint.
    _logger.addProperty(sdCardLogType);
    _logger.addProperty(serialLogType);

    _logger.setTitle("Logging");

    // sleep properties
    sleepEnabled.setTitle("Sleep");
    flxRegister(sleepEnabled, "Enable System Sleep", "If enabled, sleep the system ");
    flxRegister(sleepInterval, "Sleep Interval (sec)", "The interval the system will sleep for");
    flxRegister(wakeInterval, "Wake Interval (sec)", "The interval the system will operate between sleep period");

    startupOutputMode.setTitle("Advanced");
    flxRegister(startupOutputMode, "Startup Messages", "Level of message output at startup");
    flxRegister(startupDelaySecs, "Startup Delay", "Startup Menu Delay in Seconds");
    flxRegister(verboseDevNames, "Device Names", "Name always includes the device address");
    flxRegister(verboseEnabled, "Verbose Messages", "Enable verbose messages");

    // about?
    flxRegister(aboutApplication, "About...", "Details about the system");
    aboutApplication.prompt = false; // no prompt needed before execution

    // Log system info property - we host it at the app level, but only display it in the logger object
    // So *manually* register so it's not added to this objects property list - set name, desc
    logSysInfo.setName("System Info", "Log system information");
    logSysInfo(this, true); // set owner object - skip the add to prop list for this object

    // add this prop to our logger
    _logger.addProperty(logSysInfo);

    // Update timer object string
    _timer.setName("Logging Timer", "Set the interval between log entries");

    // set sleep default interval && event handler method
    sleepInterval = kSystemSleepSleepSec;
    _sleepJob.setup("sleep", kSystemSleepSleepSec * 1000, this, &sfeDataLogger::enterSleepMode, true);

    // app key
    flux.setAppToken(_app_jump, sizeof(_app_jump));

    // do not want the wifi connect() call made until after initialize
    _wifiConnection.setDelayedStartup(true);
}

//---------------------------------------------------------------------------
// Event callbacks
//---------------------------------------------------------------------------
// Display things during firmware loading
//---------------------------------------------------------------------------
void sfeDataLogger::onFirmwareLoad(bool bLoading)
{
    if (bLoading)
        sfeLED.on(sfeLED.Yellow);
    else
        sfeLED.off();
}

//---------------------------------------------------------------------------
// Flash led on error/warnings
//---------------------------------------------------------------------------
void sfeDataLogger::onErrorMessage(uint8_t msgType)
{
    // send an LED thing
    if (msgType == (uint8_t)flxLogError)
        sfeLED.flash(sfeLED.Red);
    else if (msgType == (uint8_t)flxLogWarning)
        sfeLED.flash(sfeLED.Yellow);
}

//---------------------------------------------------------------------------
// Display things during settings edits
//---------------------------------------------------------------------------
void sfeDataLogger::onSettingsEdit(bool bLoading)
{

    if (bLoading)
    {
        setOpMode(kDataLoggerOpEditing);
        sfeLED.on(sfeLED.LightGray);
    }
    else
    {
        sfeLED.off();

        // no longer editing
        clearOpMode(kDataLoggerOpEditing);

        // did the editing operation set a restart flag? If so see if the user wants to restart
        // the device.
        if (inOpMode(kDataLoggerOpPendingRestart))
        {
            flxLog_N("\n\rSome changes required a device restart to take effect...");
            _sysUpdate.restartDevice();

            // this shouldn't return unless user aborted
            clearOpMode(kDataLoggerOpPendingRestart);
        }
    }
}

//---------------------------------------------------------------------------
void sfeDataLogger::onSystemActivity(void)
{
    sfeLED.flash(sfeLED.Gray);
}

//---------------------------------------------------------------------------
void sfeDataLogger::onSystemActivityLow(void)
{
    sfeLED.flash(sfeLED.Blue);
}

//---------------------------------------------------------------------------
// Button Events - general handler
//---------------------------------------------------------------------------
//
// CAlled when the button is pressed and an increment time passed
void sfeDataLogger::onButtonPressed(uint32_t increment)
{

    // we need LED on for visual feedback...
    sfeLED.setDisabled(false);

    if (increment == 1)
        sfeLED.blink(sfeLED.Yellow, kLEDFlashSlow);

    else if (increment == 2)
        sfeLED.blink(kLEDFlashMedium);

    else if (increment == 3)
        sfeLED.blink(kLEDFlashFast);

    else if (increment >= 4)
    {
        sfeLED.stop();

        sfeLED.on(sfeLED.Red);
        delay(500);
        sfeLED.off();

        // Reset time !
        resetDevice();
    }
}
//---------------------------------------------------------------------------
void sfeDataLogger::onButtonReleased(uint32_t increment)
{
    if (increment > 0)
        sfeLED.off();
}

//---------------------------------------------------------------------------
// Flux flxApplication LifeCycle Method
//---------------------------------------------------------------------------
// onSetup()
//
// Called by the system before devices are loaded, and system initialized
bool sfeDataLogger::onSetup()
{

    // do we need to disable startup messages (Warn and Error still displayed)
    if (startupOutputMode() == kAppStartupMsgNone)
        flxLog.setLogLevel(flxLogWarning);

    // See if we can ID the board we're running on.
    _modeFlags |= dlModeCheckSystem();

    // Lets set the application name. If we recognize the board, we use it's name, otherwise
    // we use something generic

    if (dlModeCheckValid(_modeFlags))
        setName(dlModeCheckName(_modeFlags));
    else
        setName(dlModeCheckName(SFE_DL_IOT_9DOF_MODE)); // probably an original board

    setDescription(kDLVersionBoardDesc);

    // flxLog_I("DEBUG: onSetup() enter - Free Heap: %d", ESP.getFreeHeap());

    // Version info
    setVersion(kDLVersionNumberMajor, kDLVersionNumberMinor, kDLVersionNumberPoint, kDLVersionDescriptor, BUILD_NUMBER);

    // set the settings storage system for the framework
    flxSettings.setStorage(&_sysStorage);
    flxSettings.setFallback(&_jsonStorage);

    // Have JSON storage write/use the SD card
    _jsonStorage.setFileSystem(&_theSDCard);
    _jsonStorage.setFilename("datalogger.json");

    // Have settings saved when editing via serial console is complete.
    flxRegisterEventCB(flxEvent::kOnEdit, this, &sfeDataLogger::onSettingsEdit);
    flxRegisterEventCB(flxEvent::kOnEditFinished, &flxSettings, &flxSettingsSave::saveEvent_CB);
    flxRegisterEventCB(flxEvent::kOnNewFile, &flxSettings, &flxSettingsSave::saveEvent_CB);

    // Add serial settings to flux - the flux loop call will take care
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

    flxRegisterEventCB(flxEvent::kOnFirmwareLoad, this, &sfeDataLogger::onFirmwareLoad);

    // Add to the system - manual add so it appears last in the ops list
    _sysUpdate.setTitle("Advanced");
    flux.add(_sysUpdate);

    // The on-board button
    flux.add(_boardButton);

    // wire in LED to the logging system
    flxRegisterEventCB(flxEvent::kLogErrWarn, this, &sfeDataLogger::onErrorMessage);

    // We want an event every 5 seconds
    _boardButton.setPressIncrement(kButtonPressedIncrement);

    // Button events we're listening on
    _boardButton.on_buttonRelease.call(this, &sfeDataLogger::onButtonReleased);
    _boardButton.on_buttonPressed.call(this, &sfeDataLogger::onButtonPressed);

    // was device auto load disabled by startup commands?
    if (inOpMode(kDataLoggerOpStartNoAutoload))
        flux.setAutoload(false);

    // was settings restore disabled by startup commands?
    if (inOpMode(kDataLoggerOpStartNoSettings))
        flux.setLoadSettings(false);

    // was wifi startup disabled by startup commands?
    if (inOpMode(kDataLoggerOpStartNoWiFi))
        _wifiConnection.setDelayedStartup();

    // was wifi startup disabled by startup commands?
    if (inOpMode(kDataLoggerOpStartListDevices))
        flux.dumpDeviceAutoLoadTable();

    // setup our event callbacks for system/framework events;
    flxRegisterEventCB(flxEvent::kOnSystemActivity, this, &sfeDataLogger::onSystemActivity);
    flxRegisterEventCB(flxEvent::kOnSystemActivityLow, this, &sfeDataLogger::onSystemActivityLow);

    return true;
}

//---------------------------------------------------------------------------
// Flux flxApplication LifeCycle Method
//---------------------------------------------------------------------------
//---------------------------------------------------------------------
// onDeviceLoad()
//
// Called after qwiic/i2c auto-load, but before system state restore

void sfeDataLogger::onDeviceLoad()
{
    // quick check on fuel gauge - which is part of the IOT 9DOF board
    auto fuelGauge = flux.get<flxDevMAX17048>();
    if (fuelGauge->size() > 0)
    {
        _modeFlags |= DL_MODE_FLAG_FUEL;
        _fuelGauge = fuelGauge->at(0);
    }

    // Spin up the SPI devices for the 9DOF board? Do this if:
    //      - This a 9DOF board
    //      - This is an unknown board (b/c of legacy detection methods)

    if (!dlModeCheckValid(_modeFlags) || dlModeIsDL9DOFBoard(_modeFlags))
    {
        // Load SPI devices
        // Note - framework is setting up the pins ...

        // IMU
        if (_onboardIMU.initialize(kAppOnBoardIMUCS))
            _modeFlags |= DL_MODE_FLAG_IMU;

        // Magnetometer
        if (_onboardMag.initialize(kAppOnBoardMAGCS))
            _modeFlags |= DL_MODE_FLAG_MAG;

        // If this wasn't a known board, check if it is now?
        if (!dlModeCheckValid(_modeFlags) && dlModeCheckDevice9DOF(_modeFlags))
        {
            _modeFlags |= SFE_DL_IOT_9DOF_MODE;
            setName(dlModeCheckName(_modeFlags)); // fix name
        }
    }

#ifdef ENABLE_OLED_DISPLAY
    // OLED connected?
    auto oled = flux.get<flxDevMicroOLED>();
    if (oled->size() > 0)
    {
        _microOLED = oled->at(0);
        _pDisplay = new sfeDLDisplay();
        _pDisplay->initialize(_microOLED, &_wifiConnection, _fuelGauge, &_theSDCard);
    }
#endif

    // Was a button attached? If it is, on press event, we trigger a log event.
    // To do this, we need to *wire up* the events
    // Identify any buttons
    // Add their on_clicked events as logger triggers
    auto buttons = flux.get<flxDevButton>();

    if (buttons->size() > 0)
    {
        for (auto b : *buttons)
            _logger.listen(b->on_clicked); // Connect logger to the clicked event
    }

    // Twist?
    auto twists = flux.get<flxDevTwist>();

    if (twists->size() > 0)
    {
        for (auto tw : *twists)
            _logger.listen(tw->on_clicked); // Connect logger to the clicked event
    }

    // setup the GNSS device - will create some properties that should be visible
    // after the device is loaded and before restore (if the settings are saved)
    setupGNSS();
}
//---------------------------------------------------------------------
// onRestore()
//
// Called just before settings are restored on startup.

void sfeDataLogger::onRestore(void)
{
    // At this point, we know enough about the device to set details about it.
    char prefix[5] = "0000";
    (void)dlModeCheckPrefix(_modeFlags, prefix);
    setAppClassID(kDLAppClassNameID, prefix); // internal name string for this app type
}

//---------------------------------------------------------------------
// reset the device - erase settings, reboot
void sfeDataLogger::resetDevice(void)
{
    _sysStorage.resetStorage();
    _sysUpdate.restartDevice();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Flux flxApplication LifeCycle Method
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
///
/// @brief Checks for any before startup commands from the user
///

void sfeDataLogger::onInitStartupCommands(uint delaySecs)
{

    // Waking up from sleep?
    if (boot_count > 0)
        return;

    uint32_t delayMS = delaySecs * 1000;

    uint32_t nIterations = delayMS / kStartupLoopDelayMS;

    // setup our commands
    //
    // may a simple command table
    typedef struct
    {
        char key;
        uint16_t mode;
        const char *name;
    } startupCommand_t;
    startupCommand_t commands[] = {
        {'n', kDataLoggerOpNone, "normal-startup"},
        {'v', kAppOpStartVerboseOutput, "verbose-output-enabled"},
        {'a', kDataLoggerOpStartNoAutoload, "device-auto-load-disabled"},
        {'l', kDataLoggerOpStartListDevices, "i2c-driver-listing-enabled"},
        {'w', kDataLoggerOpStartNoWiFi, "wifi-disabled"},
        {'s', kDataLoggerOpStartNoSettings, "settings-restore-disabled"},
    };

    // Default
    int iCommand = 0;

    Serial.printf("\n\r");

    if (nIterations > 0)
    {
        // clear buffer
        while (Serial.available() > 0)
            Serial.read();

        bool menuSelected = false;
        Serial.printf("Startup Options:\n\r");
        for (int n = 0; n < sizeof(commands) / sizeof(startupCommand_t); n++)
            Serial.printf("   '%c' = %s\n\r", commands[n].key, commands[n].name);
        Serial.printf("Select Option[%2us]:", delaySecs);

        char chBS = 8; // backspace
        char chCodeBell = 7;

        for (int i = 0; i < nIterations && !menuSelected; i++)
        {
            delayMS -= kStartupLoopDelayMS;
            if (delayMS / 1000 != delaySecs)
            {
                Serial.write(chBS);
                Serial.write(chBS);
                Serial.write(chBS);
                Serial.write(chBS);
                Serial.write(chBS);
                delaySecs = delayMS / 1000;
                Serial.printf("%2us]:", delaySecs);
            }

            delay(kStartupLoopDelayMS);

            if (Serial.available() > 0)
            {
                // code /key pressed
                uint8_t chIn = Serial.read();

                // any match
                for (int n = 0; n < sizeof(commands) / sizeof(startupCommand_t); n++)
                {
                    if (chIn == commands[n].key)
                    {
                        iCommand = n;
                        menuSelected = true;
                        break;
                    }
                }
                if (!menuSelected)
                    Serial.write(chCodeBell); // bad char
            }
        }
    }
    // set the op mode
    setOpMode(commands[iCommand].mode);
    Serial.printf(" %s\n\r", commands[iCommand].name);
}
//---------------------------------------------------------------------------
// onInit()
//
// Called before the system/framework is up

void sfeDataLogger::onInit(void)
{
    // Did the user set a serial value?
    uint32_t theRate;
    uint32_t theDelay;
    getStartupProperties(theRate, theDelay);

    // just to be safe...
    theRate = theRate >= 1200 ? theRate : kDefaultTerminalBaudRate;

    Serial.begin(theRate);

    // Wait on serial - not sure if a timeout is needed ... but added for safety
    for (uint32_t startMS = millis(); !Serial && millis() - startMS <= kSerialStartupDelayMS;)
        delay(250);

    sfeLED.initialize();
    sfeLED.on(sfeLED.Green);

    setOpMode(kDataLoggerOpStartup);

    startupDelaySecs = theDelay;
    onInitStartupCommands(theDelay);
}
//---------------------------------------------------------------------------
// Check our platform status
void sfeDataLogger::checkOpMode()
{
    _isValidMode = dlModeCheckValid(_modeFlags);

    // DO we need to nag? If so, add nag job to the loop
    if (!_isValidMode)
    {
        // Create a timed job that will trigger a nag message at a regular interval
        flxJob *pJob = new flxJob;
        if (pJob != nullptr)
        {
            pJob->setup("!SparkFun", kLNagTimesMSecs, this, &sfeDataLogger::outputVMessage);
            flxAddJobToQueue(*pJob);
        }
        else
            flxLog_W(kLNagMessage);
    }
    // at this point we know the board we're running on. Set the name...
    setName(dlModeCheckName(_modeFlags));
}

//---------------------------------------------------------------------------
// Flux flxApplication LifeCycle Method
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// onStart()
//
// Called after the system is loaded, restored and initialized
bool sfeDataLogger::onStart()
{

    // flxLog_I("DEBUG: onStart() - entry -  Free Heap: %d", ESP.getFreeHeap());

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

    // init wifi
    _wifiConnection.connect();
    // Logging is done at an interval - using an interval timer.
    // Connect logger to the timer event
    _logger.listen(_timer.on_interval);

    //  - Add the JSON and CVS format to the logger
    _logger.add(_fmtJSON);
    _logger.add(_fmtCSV);

    // check SD card status
    if (!_theSDCard.enabled())
    {
        // disable SD card output
        set_logTypeSD(kAppLogTypeNone);
    }

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
            // output the connected devices ... include device type/address
            flxLog_N_(F("    %-20s  - %-40s  {"), device->name(), device->description());
            if (device->getKind() == flxDeviceKindI2C)
                flxLog_N("%s x%x}", "qwiic", device->address());
            else
                flxLog_N("%s p%u}", "SPI", device->address());

            if (device->nOutputParameters() > 0)
                _logger.add(device);
        }
    }

    // Setup the Bio Hub
    setupBioHub();

    // setup the ENS160
    setupENS160();

    // Check time devices
    if (!setupTime())
        flxLog_W(F("Time reference setup failed."));

    flxLog_N("");

    // set our system start time in milliseconds
    _startTime = millis();

    // Do we have a fuel gauge ...
    if (_fuelGauge)
    {
        _batteryJob.reset(new flxJob);
        if (_batteryJob != nullptr)
        {
            _batteryJob->setup("Battery", kBatteryCheckInterval, this, &sfeDataLogger::checkBatteryLevels);
            flxAddJobToQueue(*_batteryJob);
        }
    }

    checkOpMode();

    if (startupOutputMode() == kAppStartupMsgNormal)
        displayAppStatus(true);

    if (!_isValidMode)
        outputVMessage();

    // for our web server file search
    _iotWebServer.setFilePrefix(_theOutputFile.filePrefix());

    // clear startup flags/mode
    clearOpMode(kDataLoggerOpStartup);
    clearOpMode(kDataLoggerOpStartAllFlags);

#ifdef ENABLE_OLED_DISPLAY
    if (_pDisplay)
        _pDisplay->update();
#endif
    sfeLED.off();

    // we are done with startup - reset output mode
    if (startupOutputMode() != kAppStartupMsgNormal)
        flxLog.setLogLevel(flxLogInfo);

    // flxLog_I("DEBUG: onStart() - exit -  Free Heap: %d", ESP.getFreeHeap());

    return true;
}

//---------------------------------------------------------------------------
void sfeDataLogger::enterSleepMode()
{

    if (!sleepEnabled())
        return;

    time_t t_now;
    time(&t_now);
    struct tm *tmLocal = localtime(&t_now);
    char szBuffer[32];
    strftime(szBuffer, sizeof(szBuffer), "%d-%m-%G %T", tmLocal);

    flxLog_I(F("%s: Starting device deep sleep for %u secs"), szBuffer, sleepInterval());

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
    _logger.logMessage("INVALID PLATFORM", (char *)kLNagMessage);

    // if not logging to the serial console, dump out a message

    if (_logTypeSer == kAppLogTypeNone)
        flxLog_W(kLNagMessage);
}
//---------------------------------------------------------------------------
// checkBatteryLevels()
//
// If a battery is attached, flash the led based on state of charge
//
void sfeDataLogger::checkBatteryLevels(void)
{
    if (!_fuelGauge)
        return;

    float batterySOC = _fuelGauge->getSOC();

    sfeLEDColor_t color;

    if (batterySOC > kBatteryNoBatterySOC) // no battery
        return;

    if (batterySOC < 10.)
        color = sfeLED.Red;
    else if (batterySOC < 50.)
        color = sfeLED.Yellow;
    else
        color = sfeLED.Green;

    sfeLED.flash(color);
}

//---------------------------------------------------------------------------
// loop()
//
// Called during the operational loop of the system.

bool sfeDataLogger::loop()
{
    // key press at Serial Console? What to do??
    if (Serial.available())
    {
        // Bang command?
        uint8_t chIn = Serial.read();
        if (chIn == '!')
        {
            flxSerial.textToWhite();
            Serial.write('>');
            flxSerial.textToNormal();
            Serial.write('!');
            Serial.flush();
            sfeDLCommands cmdProcessor;
            bool status = cmdProcessor.processCommand(this);
        }
        else // edit settings
        {
            // start an editing session
            sfeLEDColor_t color;
            int status = _serialSettings.editSettings();
            if (status == -1)
                color = sfeLED.Red;
            else if (status == 1)
                color = sfeLED.Green;
            else
                color = sfeLED.Yellow;
            sfeLED.flash(color);
        }
    }
    return false;
}
