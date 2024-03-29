ESP32 Fake Light
================

This project mimics the Elgato Key Light API on an ESP32.

[Elgato's Key Lights](https://amzn.to/2GegWSc) are great, this project isn't a real replacement for them. I noticed 
there is no security for it's REST API, so I figured I'd hack up a clone with an ESP32.

## Limitations

- Just like the original there really is no security for this device. It's discoverable via mDNS, so anyone on your 
  network can turn your light on/off.
- No temperature control, I'm using single color LED strips

## Hardware

- [LED Strip Lights](https://amzn.to/2S5lixH)
- [ESP32 Dev Board](https://amzn.to/2G5tdIY)
- [MOSFET Driver](https://amzn.to/339r1J9) - To isolate the LED Strip's power
- [12V 5A Power Supply](https://amzn.to/3n6HBBB)

## Wiring

- ESP32 pin `GPIO23` to MOSFET Driver `TRIG/PWM`
- ESP32 `GRD` and MOSFET Driver `GRD` to Ground...
- LED Strip to MOSFET Driver `OUT+` and `OUT-`
- Power Supply to MOSFET Driver `VIN+` and `VIN-`
- Connect the ESP32's USB port to your computer to upload the firmware

![ESP32 Pin Map](https://github.com/espressif/arduino-esp32/raw/master/docs/esp32_pinmap.png)

**TODO** add image of wire connections

## Software

- [PlatformIO](https://docs.platformio.org/en/latest/core/installation.html)
  For macOS `brew install platformio`

## Build / Install

To compile the code, clone this repo:

```sh
git clone https://github.com/bdemers/esp32-fake-light.git
cd esp32-fake-light
```

Then build it with PlatformIO

```sh
pio run
```


Upload the firmware via USB:

```shell
pio run --target upload --upload-port /dev/cu.usbserial-0001
```

Or upload over the air:

> **NOTE:** Uncomment the `upload_flags` section in `platformio.ini` and then run:

```sh
OTA_PASS=<your-pass> pio run -t upload --upload-port <device-ip-address>
```

After uploading the firmware you MUST set your WiFi SSID and passphrase, run:

```sh
pio device monitor

# then run
wifi -ssid <your-ssid> -pass <your-pass>'

# additionally you should probably set:
hostname <your-hostname>

# set a unique mdns service name
mdns -service_name <your-service-name> -device_id <3C:6A:9D:13:C1:BD>

# enable Over The Air password updates
ota -pass <a-password>
```

This will restart your ESP32 and connect to your Wifi.  Assuming all goes well, you should see "Fake Light" listed in 
the Elgato Control Center Application.

## Serial Commands 

* **light-on \[-on <1>]**
    
    Enables or disables light

* **light-temperature \[-temp <1>]**

    Not Implemented

* **light-brightness \[-brightness <1>]**
    
    Sets light brightness as a percentage 0-100

* **mdns \[-service_name <Elgato Key Light Air 1337>] \[-device_id <3C:6A:9D:13:C1:BD>]**

    Sets the mDNS service name and device id, and restarts device

* **wifi -ssid <value> -pass <value>**

    Sets WiFi SSID and Password

* **reboot**

    Reboots device

* **echo -message <value>**

    Prints message back to terminal

* **hostname -hostname <value>**

    Updates device hostname and restarts

* **status**
    
    Prints basic device status

* **ota \[-port <3232>] -pass <value>**

    Sets the OTA firmware update port and password, then restarts

* **help**

    Prints this help message


## REST Endpoints

- `/elgato/accessory-info"` - `GET` | `PUT`
  `PUT` example:

  ```sh
  echo '{"displayName": "<your-nam>"}' | http PUT <device-ip>:9123
  ```
- `/elgato/lights/settings"` - `GET`
- `/elgato/lights` - `GET` | `PUT`

  ```sh
  # light on, brightness 100%
  echo '{"lights":[{"brightness":100,"on":1}]}' | http PUT <device-ip>:9123
  ```