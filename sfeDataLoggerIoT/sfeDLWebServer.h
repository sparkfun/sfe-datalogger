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

// index.html file
static const char *_indexHTML = R"literal(
<!DOCTYPE html>
<html>
<head>
  <title>SparkFun DataLogger IoT</title>
</head>
<body>
 <h1>Available Log Files</h1>
 <script>

  var theWS;

  function getPage(p){
    const r= {
        type: "page",
        page: p
    };
    theWS.send(JSON.stringify(r));
  }

  function setupWS(){
    theWS = new WebSocket( "ws://" + window.location.host + "/ws");
    theWS.onopen = (event) => {
        getPage(0);
    }
    theWS.onmessage = (event) => {
        console.log(event.data);
    }
  }
 window.onload= function()
 {
    setupWS();
 }
 
 </script>
 </body>
 </html>
)literal";
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

    bool setupServer(void)
    {
        flxLog_I("web server setup.");
        if (_pWebServer)
            return true;

        _pWebServer = new AsyncWebServer(80);
        if (!_pWebServer)
        {
            flxLog_E(F("%s: Failure to allocate web server"), name());
            return false;
        }
        _pWebSocket = new AsyncWebSocket("/ws");
        if (!_pWebSocket)
        {
            flxLog_E(F("%s: Failure to allocate web socket"), name());
            delete _pWebServer;
            _pWebServer = nullptr;
            return false;
        }
        // hey, we have a web server - yay
        _pWebSocket->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                                    uint8_t *data,
                                    size_t len) { return this->onEventDerived(server, client, type, arg, data, len); });

        _pWebServer->addHandler(_pWebSocket);
        // do a simple callback for now.

        _pWebServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
            flxLog_I("%s: Home Page", name());

            // request->send(200, "text/html", "Hello from the DataLogger");
            request->send(200, "text/html", _indexHTML);
        });

        flxLog_I("Web server start");
        _pWebServer->begin();
        return true;
    }
    void onEventDerived(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                        uint8_t *data, size_t len)
    {
        flxLog_I(F("In onEvent"));
        if (type == WS_EVT_CONNECT)
        {
            flxLog_I("Web Socket Connected");
        }
        else if (type == WS_EVT_DATA)
        {
            flxLog_I("Web Socket Data");
            AwsFrameInfo *info = (AwsFrameInfo *)arg;
            if (info->final && info->index == 0 && info->len == len)
            {
                // the whole message is in a single frame and we got all of it's data
                flxLog_I("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(),
                         (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

                if (info->opcode == WS_TEXT)
                {
                    data[len] = 0;
                    flxLog_I("In Message: %s", (char *)data);
                }
                DynamicJsonDocument jDoc(2000);
                int result = getFilesForPage(1, jDoc);
                if (result > 0)
                {
                    std::string sBuffer;
                    serializeJson(jDoc, sBuffer);
                    client->text(sBuffer.c_str());
                    flxLog_I("Output: %s", sBuffer.c_str());
                }
                else
                    client->text("{\"count\":0}");
                flxLog_I("Result from get files: %d", result);
            }
        }
    }

  public:
    sfeDLWebServer()
        : _theNetwork{nullptr}, _isEnabled{false}, _canConnect{false}, _fileSystem{nullptr}, _pWebServer{nullptr},
          _pWebSocket{nullptr}
    {
        flxRegister(enabled, "Enabled", "Enable or Disable the Web Server");
        setName("IoT Web Server", "Browse and Download log files on the SD Card");

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

  protected:
    flxNetwork *_theNetwork;

  private:
    int getFilesForPage(uint nPage, DynamicJsonDocument &jDoc);

    bool _isEnabled;
    bool _canConnect;

    AsyncWebServer *_pWebServer;
    AsyncWebSocket *_pWebSocket;

    // Filesystem to load a file from
    flxIFileSystem *_fileSystem;
};
