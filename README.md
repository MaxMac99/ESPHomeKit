# ESP8266 HomeKit

This repository contains a Apple HomeKit implementation for the ESP8266. It can be used iva PlatformIO.

## Installation

You can install it to your PlatformIO environment via:

```sh
platformio lib -g install https://gitlab.com/esp-homekit/esp-homekit.git
```

In your platformio.ini add HomeKit to your `lib_deps`.

## Usage

### HomeKit class

First you should create a class that has the HomeKit class.
You can initialize it with a password, a setup ID and a custom name.

The password has to be a string of format `"123-45-678"`.
You can also pass a setup ID as a string of four characters.
It is used for initial connection and QR-Code generation.
If you do not pass a name, the name is automatically generated from the name service.

After you have set up your accessory and put it in the HomeKit with `setAccessory(HKAccessory *accessory)`, you have to call `setup()` on HomeKit.
In the `update()` method of the loop call `update()` on HomeKit.

Please do not access anything on the EEPROM.
You can save the SSID with password by calling `saveSSID(String ssid, String password)`.
You can access the saved SSID and password by calling `getSSID()` or `getWiFiPassword()`.

By calling `reset()` you will reset the whole EEPROM and restart the ESP.

### HKAccessory class

You should override this class and implement the following methods:

- `run()`: This method gets called in every update cycle
- `setup()`: Setup your accessory by adding services
