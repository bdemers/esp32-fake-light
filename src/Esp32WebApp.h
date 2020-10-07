//
// Created by Brian Demers on 10/4/20.
//

#ifndef ESP32_LIGHT_ESP32WEBAPP_H
#define ESP32_LIGHT_ESP32WEBAPP_H

#include "Esp32App.h"
#include <ESPAsyncWebServer.h>

class Esp32WebApp: public Esp32App {

private:
    AsyncWebServer &server;

protected:
    void registerCommands() override;

public:
    explicit Esp32WebApp(AsyncWebServer &server);
    void begin() override;
};


#endif //ESP32_LIGHT_ESP32WEBAPP_H
