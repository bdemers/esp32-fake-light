#ifndef ESP32_LIGHT_LIGHTS_H
#define ESP32_LIGHT_LIGHTS_H

#include <ArduinoJson.h>
#include "Settings.h"

struct Light {
    uint8_t brightness;
    uint8_t on;
    uint8_t temperature; // TODO this is not a uint8, but it's not getting used

    Light() {
        Settings settings;
        brightness = settings.powerOnBrightness;
        on = settings.powerOnBehavior;
        temperature = settings.powerOnBrightness;
    }
};

struct Lights {
    uint8_t numberOfLights = 1;
    Light lights[1] = { Light() };

    Lights() = default;

    void fromJson(JsonDocument &doc) {

        if (doc["lights"]) {

            const JsonArray& lightArray = doc["lights"];
            const JsonObject& light0 = lightArray[0];

            if (light0["brightness"]) {
                uint8_t brightness = light0["brightness"].as<uint8_t>();

                if (lights[0].brightness != brightness) {
                    lights[0].brightness = brightness;
                }
            }

            if (light0["temperature"]) {
                uint8_t temperature = light0["temperature"].as<uint8_t>();

                if (lights[0].temperature != temperature) {
                    lights[0].temperature = temperature;
                }
            }

            if (light0["on"] || 0 == light0["on"]) {
                uint8_t on = light0["on"].as<uint8_t>();

                if (lights[0].on != on) {
                    lights[0].on = on;
                }
            }

        }
    }

    DynamicJsonDocument toJson() {
        DynamicJsonDocument doc(512);
        doc["numberOfLights"] = numberOfLights;

        JsonArray features = doc.createNestedArray("lights");

        DynamicJsonDocument light0(200);
        light0["brightness"] = lights[0].brightness;
        light0["on"] = lights[0].on;
        light0["temperature"] = lights[0].temperature;

        features.add(light0);
        return doc;
    }
};
#endif //ESP32_LIGHT_LIGHTS_H
