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

#include "sfeDLWebServer.h"
#include <ArduinoJson.h>
#include <Flux/flxSerial.h>
#include <Flux/flxUtils.h>
#include <time.h>

#include <ESPmDNS.h>

const int kWebServerFilesPerPage = 20;

const uint32_t kWebServerLogoutInactivity = 300000;

const char *kWebServerAuthRelm = "SFE-DataLogger";

// index.html file
static const char *_indexHTML = R"literal(
<!DOCTYPE html>
<html>
<head>
  <title>SparkFun DataLogger IoT</title>
  <style>
    body {
    font-family: Helvetica, sans-serif;
    font-size: 12px;
    color: #333;
    background-color:#fff
}
  h1 {
    text-align: left;
    color: #333;
  }
  table {
    width: 100%;
    border-collapse: collapse;
    border: 0px;
  }
  td {
    padding: 5px 5px;
    text-align: left;
    font-size: 16px;
  }
  tbody tr:hover{
    background-color: #DCDCDC;
  }
  th {
    padding: 10px 10px;
    text-align: left;
    border: 0px;
    font-size: 20px;
    font-weight: bold;
  }

  a, a:visited, a:active {
  color: #333;
  text-decoration: underline;
  font-weight: normal;
 }
.navbar {
  overflow: hidden;
  background-color: #333;
  position: relative;
  bottom: 0;
  width: 100%;
}

.navbtn {
  float: left;
  display: block;
  color: #f2f2f2;
  text-align: center;
  padding: 14px 16px;
  text-decoration: none;
  font-size: 17px;
  cursor: pointer;
  border:none;
  background-color: #333;
}
.navbtn:hover {background: #f1f1f1;color: black;}
.navbtn:active {background-color: #808080;color: white;}
.navbtn:disabled {background-color: #333;color: #808080; cursor:not-allowed; pointer-events:none;}

.main {
  padding: 16px;
  margin-bottom: 30px;
}

.parent {
    overflow: hidden;
    width: 80%;
}
.branding {
    float: right;
    width 20%;
    font-size: 17px;
    color: white;
    padding: 14px 16px;
}
 </style>
 
</head>
<body>
 <h1>Available Log Files</h1>
 <div class="parent">
  <table id="tbl">
    <thead>
      <tr>
        <th style="width:40%">File</th>
        <th>Size</th>
        <th>Date</th>
      </tr>
    </thead>
    <tbody></tbody>
  </table>
<div class="navbar">

<div class="navbar">
  <button class="navbtn" id="prev">Previous</button>
  <button class="navbtn" id="next">Next</button>  
  <div class="branding">SparkFun - DataLogger IoT</div>
</div>
</div>
 <script>
  var theWS;
  var _pg=0;
  function getPage(p){

    if (theWS.readyState == 2 || theWS.readyState == 3){
        _pg=p;
        setupWS();
        return;
    }
    const r= {
        ty: 1,
        pg: p
    };
    theWS.send(JSON.stringify(r));
  }
  function setupWS(){
    theWS = new WebSocket( "ws://" + window.location.host + "/ws");
    theWS.onopen = (event) => {
        getPage(_pg);
    }
    theWS.onmessage = (event) => {
        var res;
        try {
            var res = JSON.parse(event.data);
        }catch (err){
            console.log("results corrupt")
            return;
        }
        var o_tb = document.querySelectorAll("tbody")[0];
        var n_tb = document.createElement('tbody');
        res.files.forEach( (val) => {
            var row = document.createElement("tr");
            var d1 = document.createElement("td");
            var lnk = document.createElement("a");
            lnk.innerHTML = val.name;
            lnk.href = "/dl/" + val.name;
            lnk.download = val.name;
            lnk.title = "Download " + val.name;
            d1.appendChild(lnk);
            row.appendChild(d1);
            var d2 = document.createElement("td");
            d2.appendChild(document.createTextNode(val.size));
            row.appendChild(d2);
            var d3 = document.createElement("td");
            d3.appendChild(document.createTextNode(val.time));
            row.appendChild(d3);
            n_tb.append(row);
        });
        var tbl = document.getElementById("tbl");
        tbl.replaceChild(n_tb, o_tb);
        _pg = res.page;
        document.getElementById("prev").disabled= (_pg == 0);
        document.getElementById("next").disabled= (res.count < 20);

    }
  }
 window.onload= function()
 {
    var bn = document.getElementById("next");
    bn.addEventListener("click", (event) => {
        getPage(_pg + 1);
    });
    bn = document.getElementById("prev");    
    bn.addEventListener("click", (event) => {
        if (_pg > 0){
           getPage(_pg - 1);
        }
    });    
    setupWS();
 }
 
 </script>
 </body>
 </html>
)literal";

//-------------------------------------------------------------------------

/**
 * @brief      Check if we need to ask for auth info from the browser
 *
 * @param      request  The request from the client
 *
 * @return     true on auth okay, false on auth check failed.
 */
bool sfeDLWebServer::checkAuthState(AsyncWebServerRequest *request)
{
    // do we have some auth setup
    if (authUsername().length() > 0)
    {
        // Time to do auth - w
        // do we need to request auth? First time setting up or our logout timer transpired?
        if (!request->authenticate(authUsername().c_str(), authPassword().c_str(), kWebServerAuthRelm) || _bDoLogout)
        {
            _bDoLogout = false;
            request->requestAuthentication(kWebServerAuthRelm);
            return false;
        }
    }

    // we had activity - update out watchdog point
    _loginTicks = millis();
    return true;
}

//-------------------------------------------------------------------------
/**
 * @brief      Called to setup the internal web server
 *
 * @return     True on success
 */
bool sfeDLWebServer::setupServer(void)
{
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
    // hey, we have a web socket - yay
    _pWebSocket->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                                uint8_t *data,
                                size_t len) { return this->onEventDerived(server, client, type, arg, data, len); });

    _pWebServer->addHandler(_pWebSocket);

    if (authUsername().length() > 0)
        _pWebSocket->setAuthentication(authUsername().c_str(), authPassword().c_str());

    // do a simple callback for now.

    // for our home page - just send the home page text (above)
    _pWebServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        //
        //
        // Authorization check - if it fails return
        if (!checkAuthState(request))
            return;

        // update activity/login ticks
        _loginTicks = millis();
        request->send(200, "text/html", _indexHTML);
        onActivity.emit();
    });

    // Setup the handler for downloading file
    _pWebServer->on("/dl", HTTP_GET, [this](AsyncWebServerRequest *request) {
        //
        //
        //// Authorization check - if it fails return
        if (!checkAuthState(request))
            return;

        onActivity.emit();
        // get the file from the URL - move type to std.
        std::string theURL = request->url().c_str();
        std::string::size_type n = theURL.rfind('/');

        if (n == theURL.npos)
        {
            flxLog_I("No file in URL");
            request->send_P(404, "text/plain", "File not found");
            return;
        }

        // send the file..
        std::string theFile = theURL.substr(n);
        // flxLog_I(F("URL: %s, File: %s"), theURL.c_str(), theFile.c_str());

        FS theFS = _fileSystem->fileSystem();
        request->send(theFS, theFile.c_str(), "text/plain", true);
    });

    flxLog_I(F("%s: Web server started"), name());
    _pWebServer->begin();

    // MDNS?
    startMDNS();

    return true;
}

//-------------------------------------------------------------------------
/**
 *
 * @brief      OnEvent handler for the Asynch web server.
 *
 * @param      server  The web socket to the server
 * @param      client  The web socket to client
 * @param[in]  type    The event type
 * @param      arg     The argument
 * @param      data    The data to check
 * @param[in]  len     The length of the data
 */
void sfeDLWebServer::onEventDerived(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                                    uint8_t *data, size_t len)
{

    onActivity.emit();

    if (type == WS_EVT_CONNECT)
    {
        flxLog_D(F("%s: Web Socket Connected"), name());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        flxLog_D(F("%s: Web Socket Disconnect"));
        if (_dirRoot.isValid())
        {
            _dirRoot.close();
            _iCurrentFile = -1;
        }
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;

        if (info->final && info->index == 0 && info->len == len)
        {
            // the whole message is in a single frame and we got all of it's data
            // flxLog_E("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(),
            //          (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

            StaticJsonDocument<100> jMSG;
            if (info->opcode == WS_TEXT)
            {
                data[len] = 0;
                deserializeJson(jMSG, data);
            }
            if (!jMSG.isNull() && jMSG["ty"].as<uint>() == 1)
            {
                DynamicJsonDocument jDoc(3000);

                int result = getFilesForPage(jMSG["pg"].as<uint>(), jDoc);
                // send response to client
                if (result > 0)
                {
                    std::string sBuffer;
                    serializeJson(jDoc, sBuffer);
                    client->text(sBuffer.c_str());
                }
                else
                    client->text("{\"count\":0}");
            }
            // update activity...
            _loginTicks = millis();
        }
    }
}

//---------------------------------------------------------------------------------------

/**
 * @brief      used to reset the current file position to point 0 - start
 *
 * @return     true on success, false on failure
 */
bool sfeDLWebServer::resetFilePosition(void)
{

    if (!_fileSystem)
        return false;

    // file already open?
    if (_dirRoot)
        _dirRoot.close();

    // reset our point -- which is the end of the current block, which is block 0
    _iCurrentFile = -1;
    _dirRoot = _fileSystem->open("/", flxIFileSystem::kFileRead, false);

    if (!_dirRoot)
    {
        flxLog_E("%d: Error opening file system", name());
        return false;
    }

    if (!_dirRoot.isDirectory())
    {
        flxLog_E("%d: Filesystem root not a directory?", name());
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------------------

/**
 * @brief      Gets the files for page.
 *
 * @param[in]  nPage  The page
 * @param      jDoc   The JSON document
 *
 * @return     The number of items loaded
 */
int sfeDLWebServer::getFilesForPage(int nPage, DynamicJsonDocument &jDoc)
{
    // set parameters in our response doc.
    jDoc["count"] = 0;
    jDoc["page"] = nPage;
    if (!_fileSystem)
    {
        flxLog_E(F("No filesystem available."));
        return 0;
    }

    // files listing is forward only, so we keep track of a current open file and
    // file position. If the requested page is less than the current position, we
    // need to reset the position to the start of the file.

    int blockEnd = (nPage * kWebServerFilesPerPage) - 1;

    if (blockEnd < _iCurrentFile || !_dirRoot)
    {
        if (!resetFilePosition())
            return 0;
    }

    flxFSFile nextFile;

    // do we need to skip ahead to the point to start pulling files from?
    while (_iCurrentFile < blockEnd)
    {
        nextFile = _dirRoot.openNextFile();

        // have we run out of files? - this should be rare - no files on system, 1st page...
        if (!nextFile.isValid())
            return 0;

        // Log file
        if (validFileName(nextFile.name()))
            _iCurrentFile++; // increment over the file.

        nextFile.close();
    }

    // okay, the "next file position" should be at the start of the requested file block.
    // Let's grab our files.
    int nFound = 0;

    char szBuffer[32];
    time_t tWrite;

    JsonArray jaData = jDoc.createNestedArray("files");
    JsonObject jEntry;

    while (nFound < kWebServerFilesPerPage)
    {

        // grab *valid* files until the block is full, or we run out of files.
        nextFile = _dirRoot.openNextFile();

        // empty name == done
        if (!nextFile.isValid())
            break;

        if (!validFileName(nextFile.name()))
        {
            nextFile.close();
            continue;
        }
        // we have a valid file -- inc position
        _iCurrentFile++;

        // move our file pointer up
        tWrite = nextFile.getLastWrite();
        flx_utils::timestampISO8601(tWrite, szBuffer, sizeof(szBuffer), false);

        jEntry = jaData.createNestedObject();

        // char * cast - so JSON copies string ...
        jEntry["name"] = (char *)nextFile.name();
        flx_utils::formatByteString(nextFile.size(), 1, szBuffer, sizeof(szBuffer));
        jEntry["size"] = szBuffer;
        tWrite = nextFile.getLastWrite();
        flx_utils::timestampISO8601(tWrite, szBuffer, sizeof(szBuffer), false);
        jEntry["time"] = szBuffer;

        nextFile.close();
        nFound++;
    }

    jDoc["count"] = nFound;
    return nFound;
}
//-------------------------------------------------------------------

/**
 * @brief      Starts the mDNS service on the device
 *
 * @return     True on success
 */
bool sfeDLWebServer::startMDNS(void)
{
    if (!mDNSEnabled() || !_isEnabled || !_pWebServer)
        return false;

    if (_mdnsRunning)
        return true;

    // make sure we have a name for the service --
    if (_mdnsName.length() == 0)
        setupMDNSDefaultName();

    if (MDNS.begin(&_mdnsName[0]))
    {
        _mdnsRunning = MDNS.addService("http", "tcp", 80);
        if (!_mdnsRunning)
            MDNS.end();
    }

    if (!_mdnsRunning)
        flxLog_E("Error starting MDNS service: %s", _mdnsName.c_str());
    else
    {
        flxLog_I_(F("mDNS service started - server name: "));
        flxSerial.textToWhite();
        flxLog_N(F("%s.local"), _mdnsName.c_str());
        flxSerial.textToNormal();
    }

    return _mdnsRunning;
}

//-------------------------------------------------------------------

/**
 * @brief      shutdown the mDNS service if it's running
 */
void sfeDLWebServer::shutdownMDNS(void)
{
    if (!_mdnsRunning)
        return;

    MDNS.end();
    _mdnsRunning = false;
    flxLog_I(F("mDNS service shutdown"));
}

//-------------------------------------------------------------------

/**
 * @brief      Sets a default name for the mDSN service
 */
void sfeDLWebServer::setupMDNSDefaultName(void)
{
    // make up a name
    const char *pID = flux.deviceId();
    size_t id_len = strlen(pID);
    char szBuffer[48];
    snprintf(szBuffer, sizeof(szBuffer), "%s%s", kDefaultMDNSServiceName, (pID ? (pID + id_len - 5) : "1"));
    mDNSName = szBuffer;
}

//-------------------------------------------------------------------
// MDNS Enabled Property setter/getters
void sfeDLWebServer::set_isMDNSEnabled(bool bEnabled)
{
    // Any changes?
    if (_isMDNSEnabled == bEnabled)
        return;

    _isMDNSEnabled = bEnabled;

    // if the service isn't running or enabled, move on
    if (!_pWebServer || !_isEnabled)
        return;

    // was mDNS disabled?
    if (!_isMDNSEnabled)
        shutdownMDNS();
    else
        startMDNS();
}

//----------------------------------------------------------------
bool sfeDLWebServer::get_isMDNSEnabled(void)
{
    return _isMDNSEnabled;
}

// Enabled Property setter/getters
void sfeDLWebServer::set_MDNSName(std::string sName)
{
    _mdnsName = sName;
}

//----------------------------------------------------------------
std::string sfeDLWebServer::get_MDNSName(void)
{
    if (_mdnsName.length() == 0)
        setupMDNSDefaultName();

    return _mdnsName;
}

//----------------------------------------------------------------

/**
 * @brief      loop() - system loop routine
 *
 * @return     use this to check for logout
 */

bool sfeDLWebServer::loop()
{
    // Are we checking auth and if so, did the timeout for inactivity expire?
    if (_loginTicks > 0 && millis() - _loginTicks > kWebServerLogoutInactivity)
    {
        // Okay, we need to do an auth check next time the site is hit. flag this
        _bDoLogout = true;
        _loginTicks = 0;
    }
    return false;
}