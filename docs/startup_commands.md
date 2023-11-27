
# DataLogger IoT - Startup Command

While the DataLogger IoT platform has a variety of methods to change the runtime settings of a system, for some cases, once the system is initialized the setting that requires a change is set for the session or unavailable.

Often the case to test some issue requires setting several menu options, restarting the device and observing the results.

To help quickly address some of these issues, the DataLogger IoT implements a small set of ***Startup Commands***.

## Startup Commands

Startup Commands are just that - simple commands sent to the system during startup. When the DataLogger IoT system beings execution, it provide an opportunity to pass in a command that will change the initialization operation of the DataLogger IoT.

### Using a Startup Command

When the DataLogger IoT starts up, the following text appears on the screen for several seconds:

```sh
Startup: \
```

Where the '\\' is a *spinning* cursor.

To enter a *startup command*, the key corresponding to the command is pressed.

If no command is entered, the following *Status Message* is printed and startup commences as it normally would:

```sh
Startup: normal-startup
```

### Available Startup Commands

| Character | Status Message | Details |
|:------:|--------|-------|
| ```a``` | <nobr>"device-auto-load-disabled"</nobr> | The I2C (qwiic) device auto-load startup step is skipped
|```l``` | <nobr>"device-listing-enabled"</nobr> | Before the auto-load device table is deleted, the system prints the device name, address and priority value for the devices supported by the system|
|```w``` | <nobr>"wifi-disabled"</nobr> | The WiFi system for the device doesn't connect|
|```s``` | <nobr>"settings-restore-disabled"</nobr> | Any stored system preferences - on device or on the SD card - are not loaded during startup|
