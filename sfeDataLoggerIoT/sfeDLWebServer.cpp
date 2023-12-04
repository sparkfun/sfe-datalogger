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
#include <Flux/flxUtils.h>
#include <time.h>

#define kWebServerFilesPerPage 20

// index.html file
static const char *_indexHTML = R"literal(
<!DOCTYPE html>
<html>
<head>
  <title>SparkFun DataLogger IoT</title>
  <style>
    body {
    font-family: "Helvetica Neue", Helvetica, Arial, sans-serif;
    font-size: 12px;
    color: #333;
    background-color:#fff
}
  h1 {
    text-align: left;
    color: #333;
  }
  table {
    width: 85%;
    border-collapse: collapse;
    border: 0px;
  }
  td {
    padding: 5px 5px;
    text-align: left;
    font-size: 16px;
  }
  th {
    padding: 10px 10px;
    text-align: left;
    border: 0px;
    font-size: 20px;
    font-weight: bold;
  }

  a, a:visited, a:active {
  color: #E0311D;
  text-decoration: none;
  font-weight: normal;
 }
.navbar {
  overflow: hidden;
  background-color: #333;
  position: fixed;
  bottom: 0;
  width: 100%;
}

.navbar a {
  float: left;
  display: block;
  color: #f2f2f2;
  text-align: center;
  padding: 14px 16px;
  text-decoration: none;
  font-size: 17px;
}

.navbar a:hover {
  background: #f1f1f1;
  color: black;
}

.navbar a.active {
  background-color: #E0311D;
  color: white;
}

.main {
  padding: 16px;
  margin-bottom: 30px;
}
 </style>
</head>
<body>
<div class="navbar">
  <a href="#home" class="active">Home</a>
  <a href="#news">Previous</a>
  <a href="#contact" id="next">Next</a>
</div>
 <h1>Available Log Files</h1>
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
 <script>

  var theWS;
  var _pg=0;


  function getPage(p){
        console.log('get page');
        console.log(p);

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
        console.log(res);
        var o_tb = document.querySelectorAll("tbody")[0];
        var n_tb = document.createElement('tbody');
        res.files.forEach( (val) => {
            var row = document.createElement("tr");

            var d1 = document.createElement("td");
            d1.appendChild(document.createTextNode(val.name));
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
        console.log('return page');
        console.log(res.page);
        var _pg = res.page;
    }
  }
 window.onload= function()
 {
    var _pg = 0;
    const bn = document.getElementById("next");
    bn.addEventListener("click", (event) => {
        console.log("Page is ");
        console.log(_pg);
        getPage(_pg+1);
    });
    setupWS();
 }
 
 </script>
 </body>
 </html>
)literal";
//---------------------------------------------------------------------------------------
bool sfeDLWebServer::setupServer(void)
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
//---------------------------------------------------------------------------------------
void sfeDLWebServer::onEventDerived(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                                    uint8_t *data, size_t len)
{
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
                flxLog_I("Result from get files: %d", result);
            }
        }
    }
}
//---------------------------------------------------------------------------------------
int sfeDLWebServer::getFilesForPage(uint nPage, DynamicJsonDocument &jDoc)
{
    flxLog_I("Files for Page: %d", nPage);

    jDoc["count"] = 0;
    jDoc["page"] = nPage;
    if (!_fileSystem)
    {
        flxLog_E(F("No filesystem available."));
        return 0;
    }

    flxFSFile dirRoot = _fileSystem->open("/", flxIFileSystem::kFileRead, false);

    if (!dirRoot || !dirRoot.isDirectory())
    {
        flxLog_E(F("Error accessing SD Card"));
        return 0;
    }

    flxFSFile nextFile;
    // do we need to skip ahead?

    for (int i = 0; i < nPage * kWebServerFilesPerPage; i++)
    {
        nextFile = dirRoot.openNextFile();
        // have we run out of files? - this should be rare - no files on system, 1st page...
        if (!nextFile.isValid())
        {
            return 0;
        }
        nextFile.close();
    }

    int nFound = 0;

    char szBuffer[32];
    time_t tWrite;

    JsonArray jaData = jDoc.createNestedArray("files");

    JsonObject jEntry;
    while (true && nFound < kWebServerFilesPerPage)
    {
        nextFile = dirRoot.openNextFile();

        // empty name == done
        if (!nextFile.isValid())
            break;

        tWrite = nextFile.getLastWrite();
        flx_utils::timestampISO8601(tWrite, szBuffer, sizeof(szBuffer), false);

        jEntry = jaData.createNestedObject();

        // char * cast - so json copies string ...
        jEntry["name"] = (char *)nextFile.name();
        jEntry["size"] = nextFile.size();
        jEntry["time"] = szBuffer;

        nextFile.close();
        nFound++;
    }

    jDoc["count"] = nFound;
    return nFound;
}