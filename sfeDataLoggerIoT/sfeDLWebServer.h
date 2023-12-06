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

#include <Flux/flxCoreInterface.h>
#include <Flux/flxFS.h>
#include <Flux/flxFlux.h>
#include <Flux/flxNetwork.h>

#include <ArduinoJson.h>

// Testing
#include <ESPAsyncWebSrv.h>

class sfeDLWebServer : public flxActionType<sfeDLWebServer>
{
  private:
    // Enabled Property setter/getters
    void set_isEnabled(bool bEnabled)
    {
        // Any changes?
        if (_isEnabled == bEnabled)
            return;

        _isEnabled = bEnabled;
    }

    //----------------------------------------------------------------
    bool get_isEnabled(void)
    {
        return _isEnabled;
    }

    // mDNS Enabled Property setter/getters
    void set_isMDNSEnabled(bool bEnabled);
    bool get_isMDNSEnabled(void);

    // Name Property setter/getters
    void set_MDNSName(std::string sName);
    std::string get_MDNSName(void);
    //----------------------------------------------------------------

    // Event callback
    //----------------------------------------------------------------------------
    void onConnectionChange(bool bConnected)
    {

        _canConnect = bConnected;

        // Are we enabled ...
        if (!_isEnabled)
            return;

        setupServer();
    }
    bool setupServer(void);
    void onEventDerived(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                        uint8_t *data, size_t len);

  public:
    sfeDLWebServer()
        : _theNetwork{nullptr}, _isEnabled{false}, _isMDNSEnabled{false}, _canConnect{false}, _fileSystem{nullptr},
          _pWebServer{nullptr}, _pWebSocket{nullptr}, _mdnsName{nullptr}
    {
        setName("IoT Web Server", "Browse and Download log files on the SD Card");

        flxRegister(enabled, "Enabled", "Enable or Disable the Web Server");

        mDNSEnabled.setTitle("mDNS");
        flxRegister(mDNSEnabled, "mDNS Support", "Enable a name for the web address this device");
        flxRegister(mDNSName, "mDNS Name", "mDNS Name used for this devices address");
        flux.add(this);
    };

    ~sfeDLWebServer()
    {
    }
    // Used to register the event we want to listen to, which will trigger this
    // activity.
    void listenToConnection(flxSignalBool &theEvent)
    {
        // Register to get notified on connection changes
        theEvent.call(this, &sfeDLWebServer::onConnectionChange);
    }

    void setNetwork(flxNetwork *theNetwork)
    {
        _theNetwork = theNetwork;

        listenToConnection(theNetwork->on_connectionChange);
    }
    bool connected()
    {
        return (_isEnabled && _canConnect);
    }

    //---------------------------------------------------------
    void setFileSystem(flxIFileSystem *fs)
    {
        _fileSystem = fs;
    }

    // Properties

    // Enabled/Disabled
    flxPropertyRWBool<sfeDLWebServer, &sfeDLWebServer::get_isEnabled, &sfeDLWebServer::set_isEnabled> enabled;

    flxPropertyRWBool<sfeDLWebServer, &sfeDLWebServer::get_isMDNSEnabled, &sfeDLWebServer::set_isMDNSEnabled>
        mDNSEnabled = {false};

    flxPropertyRWString<sfeDLWebServer, &sfeDLWebServer::get_MDNSName, &sfeDLWebServer::set_MDNSName> mDNSName;

  protected:
    flxNetwork *_theNetwork;

  private:
    int getFilesForPage(uint nPage, DynamicJsonDocument &jDoc);
    void getMDNSDefaultName(void);

    bool _isEnabled;
    bool _isMDNSEnabled;
    bool _canConnect;

    AsyncWebServer *_pWebServer;
    AsyncWebSocket *_pWebSocket;

    // Filesystem to load a file from
    flxIFileSystem *_fileSystem;

    char *_mdnsName;
};
