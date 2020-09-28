#ifndef ESP32_LIGHT_SETTINGS_H
#define ESP32_LIGHT_SETTINGS_H

#include <ArduinoJson.h>

struct Settings {
    int colorChangeDurationMs = 100;
    uint8_t powerOnBehavior = 1;
    uint8_t powerOnBrightness = 20;
    uint8_t powerOnTemperature = 213;
    int switchOffDurationMs = 300;
    int switchOnDurationMs = 100;

    Settings() = default;

    DynamicJsonDocument toJson() {
        DynamicJsonDocument doc(512);
        doc["colorChangeDurationMs"] = colorChangeDurationMs;
        doc["powerOnBehavior"] = powerOnBehavior;
        doc["powerOnBrightness"] = powerOnBrightness;
        doc["powerOnTemperature"] = powerOnTemperature;
        doc["switchOffDurationMs"] = switchOffDurationMs;
        doc["switchOnDurationMs"] = switchOnDurationMs;

        return doc;
    }
};

#endif //ESP32_LIGHT_SETTINGS_H
