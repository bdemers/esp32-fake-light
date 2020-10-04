//
// Created by Brian Demers on 10/4/20.
//

#ifndef ESP32_LIGHT_ESP32APP_H
#define ESP32_LIGHT_ESP32APP_H

#include <SimpleCLI.h>

class Esp32App {

public:

    Command addCommand(const char* name, void (* callback)(cmd* c));

    void begin();

    void handleSerialInput();
};

#endif //ESP32_LIGHT_ESP32APP_H
