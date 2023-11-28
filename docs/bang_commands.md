# DataLogger IoT ! (Bang) Commands

The serial console interactive menu system of the DataLogger IoT line of products provides an intuitive, dynamic and flexible methodology to interact with the system configuration.While extremely capable, when performing common admin or debugging tasks the menu system is an impediment to the desired operation. A fast, interactive solution is needed.

To provide rapid access to common administration and debugging commands, the DataLogger IoT firmware provides a set of commands that are entered directly into the console. Starting with a "!" (bang), these commands are referred to as ***Bang Commands***.

## Available Commands

The following commands are available:

|Command | Description|
|:---|:----|
|<nobr>!factory-reset</nobr>|Starts a factory reset of the DataLogger IoT device. A [yes/no] prompt is provided before the reset is executed|
|<nobr>!reset-device-forced</nobr>|Resets the device (clears settings and restarts the device) without a prompt|
|<nobr>!reset-device</nobr>| Resets the device (clears settings and restarts the device) with a [Yes/No] prompt|
|<nobr>!clear-settings</nobr>|Clears the on-device saved settings with a [Yes/No] prompt|
|<nobr>!clear-settings-forced</nobr>|Clears the on-device saved settings without a prompt|
|<nobr>!restart</nobr>|Restarts the device with a [Yes/No] prompt|
|<nobr>!restart-forced</nobr>|Restarts the device without a prompt|
|<nobr>!json-settings</nobr>|Allows setting the device settings by passing in a JSON setting string. A complete JSON object that contains the desired settings should follow the the entry of this command. The new settings are applied to the device and saved to local storage.|
|<nobr>!log-rate</nobr>|Outputs the current log-rate of the device (milliseconds between logging transactions)|
|<nobr>!log-rate-toggle</nobr>|Toggle the on/off state of the log rate data recording by the system. This value is not persisted to on-board settings unless the settings are saved.|
|<nobr>!wifi</nobr>|Outputs the current statistics for the WiFi connection|
|<nobr>!sdcard</nobr>|Outputs the current statistics of the SD Card |
|<nobr>!devices</nobr>|Lists the currently connected devices|
|<nobr>!save-settings</nobr>|Saves the current system settings to the preference system|
|<nobr>!normal-output</nobr>|Enable the output of normal/standard messages. This is the normal mode for the DataLogger|
|<nobr>!debug-output</nobr>|Enable the output of debug messages. This value is not persistent|
|<nobr>!verbose-output</nobr>|Enable the output of verbose messages. This value is not persistent|
|<nobr>!heap</nobr>|Outputs the current statistics of the system heap memory|
|<nobr>!about</nobr>|Outputs the full *About* page of the DataLogger|
|<nobr>!help</nobr>|Outputs the available *!* commands|

### Command Usage

To use a *Bang Command*, connect to the target DAtaLogger IoT device via a serial console. and enter the command, starting with a `!` symbol. Entering any other character will launch the standard DataLogger IoT menu system.

> Note: Since the *Bang Commands* are not interactive, they also enable commanding by another device, such as a computer attached to the DataLogger IoT.
