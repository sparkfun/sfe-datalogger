# SparkFun DataLogger IoT - Build and Update
![DataLogger Builds](https://github.com/sparkfun/SparkFun_DataLogger/actions/workflows/build-datalogger-iot.yml/badge.svg)


The repository contains the various firmware applications for the SparkFun DataLogger IoT incarnations.

The firmware defines the functionally available for the supported  DataLogger IoT boards, with the core implementation of the exposed features delivered by the [SparkFun Flux Framework](https://github.com/sparkfun/SparkFun_Flux).

## Building and Uploading

Because of the size of the various network access security keys used by the DataLogger, the preferences storage partition of the standard flash data partition structure of an Arduino built ESP32 system is too small. To leverage the built-in Preferences package of the ESP32, the DataLogger uses a custom partition scheme. As such, the DataLogger firmware is build using the ```arduino-cli``` command set and uploaded using the ESP32 tool, ```esptool```.

### Install Arduino CLI

First, install the ```arduino-cli``` on your system. Details on how to do this are located [here](https://arduino.github.io/arduino-cli/0.20/installation/).

Once the CLI is installed, the following commands complete the installation and install the ESP32 Arduino platform

```sh
arduino-cli config init --additional-urls "https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"

arduino-cli core update-index

arduino-cli core install esp32:esp32
```

### Install Flux
The version of the ***SparkFun Flux*** library the DataLogger uses is linked in as a submodule to this repository. To checkout the submodule contents, issue the following command in the root folder of the repository.

```sh
git submodule update --init --recursive
```

The flux framework depends on a large number of Arduino libraries. To install all dependencies, Flux provides an installation script. This script requires the ```arduino-cli``` and is run by issuing the following command from the root folder of the DataLogger repository:

```sh
./SparkFun_Flux/install-libs.sh
```

### Build the Firmware

The following command will build the DataLogger firmware. This command is run from the root directory of this repo.

```sh

arduino-cli compile --fqbn esp32:esp32:esp32 \
            ./sfeDataLoggerIoT/sfeDataLoggerIoT.ino \
            --build-property upload.maximum_size=3145728 \
            --build-property build.flash_size=16MB \
            --build-property build.partitions=partitions \
            --build-property build.flash_mode=dio \
            --build-property build.flash_freq=80m \
            --export-binaries --clean --library `pwd`/SparkFun_Flux 
```

Note:

* The above command is broken up to improve readability - thus the use of "\" for line continuations.
* The ```--library``` switch is used to indicate the use of the Flux library submodule.
* Build results are placed in ```./sfeDataLoggerIoT/build/build/esp32.esp32.esp32```

The result of the builds are the following files:

* ```sfeDataLoggerIoT.ino.bin``` - The Firmware binary
* ```sfeDataLoggerIoT.ino.bootloader.bin``` - The Bootloader to use for the firmware
* ```sfeDataLoggerIoT.ino.partitions.bin``` - The Flash partition map for the DataLogger

#### Automated Builds

The firmware is also automatically built by a GitHub action on code check-in. The latest files are available as build artifacts, and labeled releases.

## Firmware Upload

### esptool.py
To upload the firmware, the ESP32 tool ```esptool.py``` is used. Installation instructions are located [here](https://docs.espressif.com/projects/esptool/en/latest/esp32/installation.html). If Python is setup on your system, a ```pip3``` can be used:

```sh
pip3 install esptool
```

Once installed - the following command is used to upload the build firmware to an attached DataLogger. 

```sh
esptool.py --chip esp32 --port "<PORT>" --baud  460800 \
    --before default_reset --after hard_reset write_flash -e -z \
    --flash_mode dio --flash_freq 80m --flash_size 16MB \
    0x1000 "./sfeDataLoggerIoT/build/esp32.esp32.esp32/sfeDataLoggerIoT.ino.bootloader.bin" \
    0x8000 "./sfeDataLoggerIoT/build/esp32.esp32.esp32/sfeDataLoggerIoT.ino.partitions.bin" \
    0x20000 "./sfeDataLoggerIoT/build/esp32.esp32.esp32/sfeDataLoggerIoT.ino.bin"
```

**NOTE:**

* ```<PORT>``` - Is the port the board is connected to on your development machine
* ```-baud``` - Set this to a value that works for your development machine. The above command uses ```460800```
* ```-e``` - This command switch will release the ESP32 flash and is helpful for development. 
  * For a faster upload, omit this option

### Uploading Automated GitHub Build Results

This repository builds DataLogger firmware using a GitHub action. To use this pre-built firmware on a DataLogger IoT board, do the following steps:

Download the desired firmware files from: 

* The results from a GitHub [Action](https://github.com/sparkfun/SparkFun_DataLogger/actions)
* Or a specific DataLogger [Release](https://github.com/sparkfun/SparkFun_DataLogger/releases)

The downloaded zip file contains the three ```.bin``` files for the release. 

Once unzipped, modified the paths to the desired ```.bin``` files in the above ```esptool.py``` command to upload the firmware to an attached DataLogger board. 
