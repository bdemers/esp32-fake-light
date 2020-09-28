#ifndef ESP32_LIGHT_ACCESSORYINFO_H
#define ESP32_LIGHT_ACCESSORYINFO_H

#include <ArduinoJson.h>

struct AccessoryInfo {
    String displayName = "Fake Light";
    String features[1] = { "lights" };
    int firmwareBuildNumber = 195;
    String firmwareVersion = "1.0.3";
    int hardwareBoardType = 200;
    String productName = "Elgato Key Light Air";
    String serialNumber = "CW31J1A00183";

    AccessoryInfo() = default;

    void fromJson(JsonDocument &doc) {

        if (doc["displayName"]) {
            String newName = doc["displayName"];
            displayName = newName;
        }
    }

    DynamicJsonDocument toJson() {
        DynamicJsonDocument doc(512);
        doc["displayName"] = displayName;
        doc["firmwareBuildNumber"] = firmwareBuildNumber;
        doc["firmwareVersion"] = firmwareVersion;
        doc["hardwareBoardType"] = hardwareBoardType;
        doc["productName"] = productName;
        doc["serialNumber"] = serialNumber;

        // create an empty array
        JsonArray features = doc.createNestedArray("features");
        features.add(this->features[0]);

        return doc;
    }
};

#endif //ESP32_LIGHT_ACCESSORYINFO_H
