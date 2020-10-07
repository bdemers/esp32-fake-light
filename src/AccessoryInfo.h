#ifndef ESP32_LIGHT_ACCESSORYINFO_H
#define ESP32_LIGHT_ACCESSORYINFO_H

#include <ArduinoJson.h>
#include "FakeLight.h"

struct AccessoryInfo {
    String displayName = DEFAULT_DISPLAY_NAME;
    String features[1] = { "lights" };
    int firmwareBuildNumber = 199;
    String firmwareVersion = "1.0.3";
    int hardwareBoardType = 200;
    String productName = "Elgato Key Light Air";
    String serialNumber = "CW31J1A00183";

    AccessoryInfo() = default;

    void fromJson(JsonObject &doc) {

        if (doc["displayName"]) {
            String newName = doc["displayName"];
            displayName = newName;
        }
    }

    void toJson(JsonObject &doc) {
        doc["displayName"] = displayName;
        doc["firmwareBuildNumber"] = firmwareBuildNumber;
        doc["firmwareVersion"] = firmwareVersion;
        doc["hardwareBoardType"] = hardwareBoardType;
        doc["productName"] = productName;
        doc["serialNumber"] = serialNumber;

        // create an empty array
        JsonArray featuresNode = doc.createNestedArray("features");
        featuresNode.add(this->features[0]);
    }
};

#endif //ESP32_LIGHT_ACCESSORYINFO_H
