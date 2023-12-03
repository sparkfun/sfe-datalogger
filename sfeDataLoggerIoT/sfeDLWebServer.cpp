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

int sfeDLWebServer::getFilesForPage(uint nPage, DynamicJsonDocument &jDoc)
{
    jDoc["count"] = 0;
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

    std::string filename;
    size_t dot;

    std::string blank = "";

    int nFound = 0;

    flxFSFile nextFile;
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

        flxLog_I("File: %s\tSize: %d date: %s", nextFile.name(), nextFile.size(), szBuffer);
        // dot = filename.find_last_of(".");
        // if (dot == std::string::npos)
        //     continue; // no file extension

        // if (filename.compare(dot + 1, strlen(kFirmwareFileExtension), kFirmwareFileExtension))
        //     continue;

        // // We have a bin file, how about a firmware file - check against our prefix if one set
        // // Note adding one to filename  - it always starts with "/"

        // if (_firmwareFilePrefix.length() > 0 &&
        //     strncmp(_firmwareFilePrefix.c_str(), filename.c_str() + 1, _firmwareFilePrefix.length()) != 0)
        //     continue; // no match

        // // We have a match
        // dataLimit.addItem(blank, filename);
        nextFile.close();
        nFound++;
    }

    jDoc["count"] = nFound;
    return nFound;
}