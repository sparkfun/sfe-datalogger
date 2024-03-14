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

#include <Flux/flxCoreInterface.h>
#include <Flux/flxCoreJobs.h>
#include <Flux/flxFS.h>
#include <Flux/flxFileRotate.h>
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

        // did we move to disable the server?
        if (!_isEnabled)
        {
            // did we disable, but have a file open?
            if (_dirRoot.isValid())
            {
                _dirRoot.close();
                _iCurrentFile = 1;
            }
            // shutdown the server
            shutdownServer();
        }
        else if (_canConnect) // start server if network is up
            setupServer();
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
    void shutdownServer(void);
    void onEventDerived(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                        uint8_t *data, size_t len);

  public:
    sfeDLWebServer()
        : _theNetwork{nullptr}, _isEnabled{false}, _isMDNSEnabled{false}, _canConnect{false}, _wasShutdown{false},
          _fileSystem{nullptr}, _pWebServer{nullptr}, _pWebSocket{nullptr}, _mdnsName{""}, _mdnsRunning{false},
          _sPrefix("sfe"), _iCurrentFile{-1}, _loginTicks{0}, _bDoLogout{true}
    {
        setName("IoT Web Server", "Browse and Download log files on the SD Card");

        flxRegister(enabled, "Enabled", "Enable or Disable the Web Server");

        authUsername.setTitle("Authentication");
        flxRegister(authUsername, "Username", "Web access control. Leave empty to disable authentication");
        flxRegister(authPassword, "Password", "Web access control");

        mDNSEnabled.setTitle("mDNS");
        flxRegister(mDNSEnabled, "mDNS Support", "Enable a name for the web address this device");
        flxRegister(mDNSName, "mDNS Name", "mDNS Name used for this devices address");
        flux.add(this);
    };

    ~sfeDLWebServer()
    {
    }

    void setNetwork(flxNetwork *theNetwork)
    {
        _theNetwork = theNetwork;

        flxRegisterEventCB(flxEvent::kOnConnectionChange, this, &sfeDLWebServer::onConnectionChange);
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

    bool mdnsRunning(void)
    {
        return _mdnsRunning;
    }

    void setFilePrefix(std::string sPrefix)
    {
        if (sPrefix.length() == 0)
            return;

        _sPrefix = sPrefix;
    }

    // Properties

    // Enabled/Disabled
    flxPropertyRWBool<sfeDLWebServer, &sfeDLWebServer::get_isEnabled, &sfeDLWebServer::set_isEnabled> enabled;

    flxPropertyRWBool<sfeDLWebServer, &sfeDLWebServer::get_isMDNSEnabled, &sfeDLWebServer::set_isMDNSEnabled>
        mDNSEnabled = {false};

    flxPropertyRWString<sfeDLWebServer, &sfeDLWebServer::get_MDNSName, &sfeDLWebServer::set_MDNSName> mDNSName;

    flxPropertyString<sfeDLWebServer> authUsername;
    flxPropertySecureString<sfeDLWebServer> authPassword;

  protected:
    flxNetwork *_theNetwork;

  private:
    /**
     * @brief      Check if this is a valid log file name
     *
     * @param[in]  szName  The name
     *
     * @return     True if valid, false if not
     */
    inline bool validFileName(const char *szName)
    {

        // filenames should be <prefix><numbers>.<kLogFileSuffix>
        if (!szName || strlen(szName) < 5 || _sPrefix.length() == 0)
            return false;

        size_t slen = strlen(szName);

        // start with prefix
        if (strnstr(szName, _sPrefix.c_str(), slen) != szName)
            return false;

        char *szTmp = strnstr(szName, flxFileRotate::kLogFileSuffix, slen);

        if (!szTmp || szTmp - szName + strlen(flxFileRotate::kLogFileSuffix) != slen)
            return false;

        return true;
    }

    static constexpr char *kDefaultMDNSServiceName = "datalogger";

    bool checkAuthState(AsyncWebServerRequest *request);
    bool resetFilePosition(void);
    int getFilesForPage(int nPage, DynamicJsonDocument &jDoc);
    bool startMDNS(void);
    void shutdownMDNS(void);
    void setupMDNSDefaultName(void);
    void checkLogin(void);

    bool _isEnabled;
    bool _isMDNSEnabled;
    bool _canConnect;
    bool _wasShutdown; // we can't fully shutdown and restart the server.

    AsyncWebServer *_pWebServer;
    AsyncWebSocket *_pWebSocket;

    // Filesystem to load a file from
    flxIFileSystem *_fileSystem;

    // char *_mdnsName;
    std::string _mdnsName;
    bool _mdnsRunning;
    std::string _sPrefix;

    // current file position things

    flxFSFile _dirRoot;
    int _iCurrentFile;

    uint32_t _loginTicks;
    bool _bDoLogout;

    flxJob _jobCheckLogin;
};
