# SparkFun DataLogger IoT - Build and Update

![DataLogger Builds](https://github.com/sparkfun/SparkFun_DataLogger/actions/workflows/build-datalogger-iot.yml/badge.svg)

The repository contains the various firmware applications for the SparkFun DataLogger IoT incarnations.

The firmware defines the functionally available for the supported  DataLogger IoT boards, with the core implementation of the exposed features delivered by the [SparkFun Flux SDK](https://github.com/sparkfun/flux-sdk).

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

### Install the *flux sdk*

The flux sdk is used to create a custom Arduino Library called ```SparkFun_DataLoggerIoT``` that is made available for use by the Arduino build environment. This allows tailoring which components are needed for the specific application - in this case the datalogger.

First steps is to download the flux-sdk on your machine - which is cloned from github.

```sh
git clone git@github.com:sparkfun/flux-sdk.git
```

The normal location is to set install the flux-sdk in the same directory as the ```sfe-dataloger``` (this) repository was installed.  If you install it some other location, set the flux-sdk path using the FLUX_SDK_PATH environment variable. This is easily done by cd'ing into the flux-sdk root directory and executing the following command:

```sh
export FLUX_SDK_PATH=`pwd`
```

### Install dependant Arduino libraries

Once the flux-sdk is installed, you can install all libraries the framework depends on by issuing the following command:

```sh
$FLUX_SDK_PATH/install-libs.sh
```

### Using CMake

To configure the library used during the Arduino build process, the ```cmake``` system is used. The following steps outline how the custom library is built.

Set the current directory the root of the sfe-datalogger. Then create a directory called build and cd into it.

```sh
mkdir build
cd build
```

Now run CMake with the following command:

```sh
cmake ..
```

This will create an Arduino library called ```SparkFun_DataLoggerIoT``` in the root directory of the sfe-datalogger repository. Once completed, you can delete the build directory, and build the datalogger firmware.

### Build the Firmware

The following command will build the DataLogger firmware. This command is run from the root directory of this repo.

```sh

arduino-cli compile --fqbn esp32:esp32:esp32 \
            ./sfeDataLoggerIoT/sfeDataLoggerIoT.ino \
            --build-property upload.maximum_size=4980736 \
            --build-property build.flash_size=16MB \
            --build-property build.partitions=partitions \
            --build-property build.flash_mode=dio \
            --build-property build.flash_freq=80m \
            --export-binaries --clean --library `pwd`/SparkFun_DataLoggerIoT 
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
