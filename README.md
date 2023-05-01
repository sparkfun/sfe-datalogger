# SparkFun DataLogger IoT (sfe-datalogger)

![DataLogger Builds](https://github.com/sparkfun/sfe-datalogger/actions/workflows/build-datalogger-iot.yml/badge.svg)

The repository contains the various firmware applications for the SparkFun DataLogger IoT incarnations. This is an **internal** implementation private repository used to develop, build and release the Firmware for the DataLogger IoT boards.

Public release of this firmware and documentation are made via the public repository [SparkFun_DataLogger](https//github.com/sparkfun/SparkFun_DataLogger)

The firmware defines the functionally available for the supported  DataLogger IoT boards, with the core implementation of the exposed features delivered by the [SparkFun Flux Framework](https://github.com/sparkfun/SparkFun_Flux).

### Firmware Building and Uploading

To build and upload the SparkFun DataLogger firmware, refer to the [Build and Update Instructions](docs/build_update.md)

## DataLogger Functionality

The following sections provide details for the main features exposed by the DataLogger IoT

The devices supported by the DataLogger are listed in the [Device List](docs/supported_devices.md)

### Application Settings
* [Details on the systems Application Settings menu](https://github.com/sparkfun/sfe-datalogger/blob/main/docs/act_app_settings.md)

### IoT Service Connections

* [Connecting to ThingSpeak](https://github.com/sparkfun/SparkFun_Flux/blob/main/docs/iot_thingspeak.md)
* [Connecting to AWS IoT](https://github.com/sparkfun/SparkFun_Flux/blob/main/docs/iot_aws.md)
* [Connecting to Azure IoT](https://github.com/sparkfun/SparkFun_Flux/blob/main/docs/iot_azure.md)
* [Connecting to an HTTP Server](https://github.com/sparkfun/SparkFun_Flux/blob/main/docs/iot_http.md)
* [Connecting to an MQTT Broker](https://github.com/sparkfun/SparkFun_Flux/blob/main/docs/iot_mqtt.md)

### System

* [Resetting and Updating Device Firmware](https://github.com/sparkfun/SparkFun_Flux/blob/main/docs/act_sysfirmware.md)
