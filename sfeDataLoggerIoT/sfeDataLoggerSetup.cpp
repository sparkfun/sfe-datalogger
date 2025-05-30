
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
 * SparkFun Data Logger - setup methods
 *
 */
#include "sfeDLBoard.h"
#include "sfeDataLogger.h"

#include <Flux/flxDevBME280.h>
#include <Flux/flxDevENS160.h>
#include <Flux/flxDevGNSS.h>
#include <Flux/flxDevRV8803.h>
#include <Flux/flxDevSHTC3.h>
#include <Flux/flxDevSerial.h>

// Biometric Hub -- requires pins to be set on startup
static const uint8_t kAppBioHubReset = 17; // Use the TXD pin as the bio hub reset pin
static const uint8_t kAppBioHubMFIO = 16;  // Use the RXD pin as the bio hub mfio pin

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

        // have the CSV format driver listen to the new file event. This
        // will cause a header to be written next cycle.
        flxRegisterEventCB(flxEvent::kOnNewFile, &_fmtCSV, &flxFormatCSV::output_header);

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

    // Arduino IoT
    _iotArduinoIoT.setNetwork(&_wifiConnection);
    _fmtJSON.add(_iotArduinoIoT);

    // Web server
    _iotWebServer.setTitle("Preview");
    _iotWebServer.setNetwork(&_wifiConnection);
    _iotWebServer.setFileSystem(&_theSDCard);

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
void sfeDataLogger::setupENS160(void)
{

    // do we have one attached?
    auto ens160Devices = flux.get<flxDevENS160>();
    if (ens160Devices->size() == 0)
        return;

    flxDevENS160 *pENS160 = ens160Devices->at(0);

    auto bmeDevices = flux.get<flxDevBME280>();
    if (bmeDevices->size() > 0)
    {
        flxDevBME280 *pBME = bmeDevices->at(0);

        pENS160->setTemperatureCompParameter(pBME->temperatureC);
        pENS160->setHumidityCompParameter(pBME->humidity);

        flxLog_I(F("%s: compensation values applied from %s"), pENS160->name(), pBME->name());
        return;
    }
    // do we have a SHTC3 attached
    auto shtc3Devices = flux.get<flxDevSHTC3>();
    if (shtc3Devices->size() > 0)
    {
        flxDevSHTC3 *pSHTC3 = shtc3Devices->at(0);

        pENS160->setTemperatureCompParameter(pSHTC3->temperatureC);
        pENS160->setHumidityCompParameter(pSHTC3->humidity);

        flxLog_I(F("%s: compensation values applied from %s"), pENS160->name(), pSHTC3->name());
        return;
    }
}
void sfeDataLogger::gnssPPSEventCB(void)
{
    flxSendEvent(flxEvent::kOnLogObservationWithSource, "PPS");
}
//---------------------------------------------------------------------------
void sfeDataLogger::setupGNSS(void)
{
    // do we have one attached?
    auto gnssDevices = flux.get<flxDevGNSS>();
    if (gnssDevices->size() == 0)
        return;

    // get the first GNSS device and set the PPS Pin that can be used
    flxDevGNSS *pGNSS = gnssDevices->at(0);
    if (!pGNSS)
        return;

    // set the in we use for PPS -- expect the input to be wired to this
    pGNSS->setAvailablePPSPins(kDLBoardGNSSPPSPins, sizeof(kDLBoardGNSSPPSPins) / sizeof(kDLBoardGNSSPPSPins[0]));

    // wire in the event to the logger
    flxRegisterEventCB(flxEvent::kOnGNSSPPSEvent, this, &sfeDataLogger::gnssPPSEventCB);
}
//---------------------------------------------------------------------------
// setup serial device things
void sfeDataLogger::serialDataEventCB(void)
{
    flxSendEvent(flxEvent::kOnLogObservationWithSource, "SERIAL");
}
//---------------------------------------------------------------------------
void sfeDataLogger::setupSerial(void)
{
    // wire in the event to the logger
    flxRegisterEventCB(flxEvent::kOnSerialDataAvailable, this, &sfeDataLogger::serialDataEventCB);
}