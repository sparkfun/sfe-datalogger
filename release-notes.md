
# DataLogger IoT Release Notes

## Version 1.1.0

November 15th, 2023

### New Features

* Support for the Arduino IoT Cloud, enabling data posting to this IoT platform natively from a DataLogger IoT.
* Support for the SparkFun Indoor Air Quality Sensor - ENS160
* Support for the SparkFun Photoacoustic Spectroscopy CO2 Sensor - PASCO2V01
* Support for the SparkFun Human Presence and Motion Sensor - STHS34PF80
* Support for the SparkFun Tristimulus Color Sensor - OPT4048DTSR

### Enhancements

* Update to reference clock management - Timezone support is at the clock level, not tied to NTP
* The JSON output buffer size is now user configurable via the settings menu
* The ADS1015 driver now has configurable output data types.
* Device address values can now be added to a device name if desired.

### Bug Fixes

* Incorporated fix of the RV8803 RTC Clock Arduino library that forced time shifts based timezone offset during clock updates.
* Improved runtime memory (ram) management.
* Resolved issue with device name collision. Now, if a device name already exists, the address of the new devices is added to its name, providing a unique name value.
* Save/Restore of settings to a JSON file could overflow the internal JSON buffer. The JSON data buffer for fallback settings save/restore is now user settable.
* Improvements to the JSON document posted to Machinechat servers.

## Version 1.0.4

August 23rd, 2023

### New Features

* Support for the Base DataLogger IoT board
* Driver support for the SparkFun Photoacoustic Spectroscopy CO2 Sensor - PASCO2V01 (Qwiic) (but board not released yet )

## Version 1.0.3

July 28th, 2023

### Updates

& Fix issue with GNSS device driver.

## Version 1.0.2

May 15th, 2023

### Updates

* Improved implementation of the on-board RGB LED functionality - now supports a behavior stack and the ability to completely disable LED output.
* Support for multiple sets of WiFi Credentials - up to 4 values are now supported
* If running on a battery, RGB LED indicator of battery level every 90 seconds.  Flash based on level: Green = > 50%, Yellow  = > 10%, Red = < 10 %
* User setting to set the baud rate of the serial console output
* User setting to set the menu system timeout value
* System reset when holding down the user D0/Boot button for 20 seconds. Will erase on-board preference storage and restart the board.

## Version 1.0.1

May 2nd, 2023

### Updates

* Issue - fixed - MQTT connection would not reconnect to broker if broker went down
* Issue - fixed - Output Headers for CSV were being added to output log files after each reboot - even if using existing file
* Issue - fixed - CSV output to serial console would write headers twice when starting with a new, never initialized board/SD card
* Feature - added Mime type at the start of the serial console output
