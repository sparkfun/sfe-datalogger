
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

### Bug Fixes

* Incorporated fix of the RV8803 RTC Clock Arduino library that forced time shifts based timezone offset during clock updates.
* Improved runtime memory (ram) management.
* Resolved issue with device name collision. Now, if a device name already exists, the address of the new devices is added to its name, providing a unique name value.
* Save/Restore of settings to a JSON file could overflow the internal JSON buffer. The JSON data buffer for fallback settings save/restore is now user settable.
