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

#include "sfeDataLogger.h"

#include <Flux/flxCoreLog.h>
#include <Flux/flxCoreParam.h>
#include <Flux/flxUtils.h>

class sfeDLSystemOp : public flxOperation
{
  private:
    std::string get_wifi_ssid(void)
    {
        std::string sTmp;

        if (_pDataLogger && _pDataLogger->_wifiConnection.enabled())
            sTmp = _pDataLogger->_wifiConnection.connectedSSID().c_str();

        return sTmp;
    }

    uint8_t get_wifi_rssi(void)
    {
        if (!_pDataLogger || !_pDataLogger->_wifiConnection.enabled())
            return 0;

        return _pDataLogger->_wifiConnection.RSSI();
    }

    uint32_t get_uptime(void)
    {
        return millis();
    }

    uint32_t get_sdfree(void)
    {
        if (!_pDataLogger || !_pDataLogger->_theSDCard.enabled())
            return 0;

        return _pDataLogger->_theSDCard.total() - _pDataLogger->_theSDCard.used();
    }

    uint32_t get_heap(void)
    {
        return ESP.getFreeHeap();
    }

  public:
    sfeDLSystemOp() : _pDataLogger{nullptr}
    {
        setName("System Info", "Operating information for the DataLogger");

        flxRegister(wifiSSID, "SSID", "Current WiFi SSID");
        flxRegister(wifiRSSI, "RSSI", "Current WiFi RSSI");
        flxRegister(systemUptime, "Uptime", "System Uptime in MS");
        flxRegister(systemHeap, "Heap", "Heap free size");
        flxRegister(systemSDFree, "SD Free", "SD Card free space");
    }

    sfeDLSystemOp(sfeDataLogger *dlApp) : sfeDLSystemOp()
    {
        setDataLogger(dlApp);
    }

    void setDataLogger(sfeDataLogger *dlApp)
    {
        _pDataLogger = dlApp;
    }

    flxParameterOutString<sfeDLSystemOp, &sfeDLSystemOp::get_wifi_ssid> wifiSSID;

    flxParameterOutUInt8<sfeDLSystemOp, &sfeDLSystemOp::get_wifi_rssi> wifiRSSI;

    flxParameterOutUInt32<sfeDLSystemOp, &sfeDLSystemOp::get_uptime> systemUptime;

    flxParameterOutUInt32<sfeDLSystemOp, &sfeDLSystemOp::get_sdfree> systemSDFree;

    flxParameterOutUInt32<sfeDLSystemOp, &sfeDLSystemOp::get_heap> systemHeap;

  private:
    sfeDataLogger *_pDataLogger;
};
