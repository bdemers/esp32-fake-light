//
// Created by Brian Demers on 10/4/20.
//

#ifndef ESP32_LIGHT_ESP32APP_H
#define ESP32_LIGHT_ESP32APP_H

#include <SimpleCLI.h>
#include <esp_task.h>

class Esp32App {

protected:
    virtual void registerCommands();

public:

    Command addCommand(const char* name, void (* callback)(cmd* c));

    static void defaultNoWifiHandler() {
        Serial.println("WiFi not configured use 'wifi -ssid <your-ssid> -pass <your-pass>'");
    }

    virtual void begin();
};

#endif //ESP32_LIGHT_ESP32APP_H
