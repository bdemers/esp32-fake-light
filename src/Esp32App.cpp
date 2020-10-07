//
// Created by Brian Demers on 10/4/20.
//

#include "Esp32App.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoOTA.h>

static SimpleCLI simpleCli;

TaskHandle_t baseAppTask;

Command Esp32App::addCommand(const char *name, void (*callback)(cmd *)) {
    return simpleCli.addCommand(name, callback);
}

void wifiCommandCallback(cmd* c) {
    Command cmd(c);

    String ssid = cmd.getArg("ssid").getValue();
    String pass = cmd.getArg("pass").getValue();

    Preferences prefs;
    prefs.begin("network", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();

    Serial.println("\r\nWi-Fi credentials changed, restarting...");
    ESP.restart();
}

void rebootCommandCallback(cmd* c) {
    ESP.restart();
}

void echoCallback(cmd* c) {
    Command cmd(c);
    Serial.println();
    Serial.println(cmd.getArg().getValue());
}

void hostnameCommandCallback(cmd* c) {
    Command cmd(c);

    String hostname = cmd.getArg("hostname").getValue();
    Preferences prefs;
    prefs.begin("network", false);
    prefs.putString("hostname", hostname);
    prefs.end();

    Serial.println("\r\nHostname changed, restarting...");
    ESP.restart();
}

void statusCommandCallback(cmd* c) {
    Command cmd(c);

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("\nWiFi connected to: ");
        Serial.println(WiFi.SSID());
        Serial.print("\tHostname: ");
        Serial.println(WiFi.getHostname());
        Serial.print("\tIP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi not connected");
    }
}

void helpCommandCallback(cmd* c) {
    Serial.println("\nUsage:");
    Serial.println(simpleCli.toString());
}

void otaCommandCallback(cmd* c) {
    Command cmd(c);
    int port = cmd.getArg("port").getValue().toInt();
    String pass = cmd.getArg("pass").getValue();

    Preferences prefs;
    prefs.begin("ota", false);
    prefs.putInt("port", port);
    prefs.putString("pass", pass);
    prefs.end();

    Serial.println("Updated OTA settings");
    ESP.restart();

}

void setupWifi(const char* ssid, const char* pass, const char* hostname) {

    Serial.print("Connecting to WiFi SSID: ");
    Serial.println(ssid);

    // configure wifi
    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, pass);
    WiFi.setHostname(hostname);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());
}

void Esp32App::registerCommands() {
    Serial.println("Registering Commands");

    // wifi
    Command wifiCommand = simpleCli.addCmd("wifi", wifiCommandCallback);
    wifiCommand.setDescription("Sets WiFi SSID and Password");
    wifiCommand.addArg("ssid");
    wifiCommand.addArg("pass");

    // reboot
    simpleCli.addCmd("reboot", rebootCommandCallback)
        .setDescription("Reboots device");

    // echo
    Command echo = simpleCli.addCommand("echo", echoCallback);
    echo.addPositionalArgument("message");
    echo.setDescription("Prints message back to terminal");

    // set hostname
    Command hostnameCommand = simpleCli.addCmd("hostname", hostnameCommandCallback);
    hostnameCommand.addPositionalArgument("hostname");
    hostnameCommand.setDescription("Updates device hostname and restarts");

    // status
    simpleCli.addCommand("status", statusCommandCallback)
        .setDescription("Prints basic device status");

    // ota
    Command otaCommand = simpleCli.addCommand("ota", otaCommandCallback);
    otaCommand.setDescription("Sets the OTA firmware update port and password, then restarts");
    otaCommand.addArg("port", "3232");
    otaCommand.addArg("pass");

    // help
    simpleCli.addCommand("help", helpCommandCallback)
        .setDescription("Prints this help message");
}

String input;
void _handleSerialInput() {
    // Check if user typed something into the serial monitor
    if (Serial.available()) {
        char c = Serial.read();

        if (c == '\b') {
            input = input.substring(0, input.length() - 1);
        } else {
            input += c;
        }

        Serial.print(c);
        if (c == '\r') {
            simpleCli.parse(input);
            input = "";
        }
    }

    // Check for parsing errors
    if (simpleCli.errored()) {
        // Get error out of queue
        CommandError cmdError = simpleCli.getError();

        // Print the error
        Serial.print("ERROR: ");
        Serial.println(cmdError.toString());

        // Print correct command structure
        if (cmdError.hasCommand()) {
            Serial.print("Did you mean \"");
            Serial.print(cmdError.getCommand().toString());
            Serial.println("\"?");
        }
        Serial.print("\r\n# ");
    }
}

void loopHandler(void * pvParameters) {
    while (true) {
        _handleSerialInput();
        ArduinoOTA.handle();
    }
}

void startOTA() {

    Preferences prefs;
    prefs.begin("ota", false);
    int port = prefs.getInt("port", 3232);
    String pass = prefs.getString("pass", "admin");
    prefs.end();
    Serial.flush();

    ArduinoOTA.setPort(port);
    ArduinoOTA.setHostname(WiFi.getHostname());
    ArduinoOTA.setPassword(pass.c_str());

    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            Serial.println("Start firmware update: " + type);
        })
        .onEnd([]() {
            Serial.println("\nFirmware update finished");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("OTA firmware update failed: Authentication Error");
            else if (error == OTA_BEGIN_ERROR) Serial.println("OTA firmware update failed: Failed to Start");
            else if (error == OTA_CONNECT_ERROR) Serial.println("OTA firmware update failed: Connection Failure");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA firmware update failed: Receive Error");
            else if (error == OTA_END_ERROR) Serial.println("OTA firmware update failed: End Failed");
        });

    ArduinoOTA.begin();
}

void Esp32App::begin() {

    // register CLI commands
    registerCommands();

    // setup wifi
    Preferences prefs;
    prefs.begin("network", false);
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");
    String hostname = prefs.getString("hostname", WiFi.getHostname());
    prefs.end();
    Serial.flush();

    Serial.println("Checking for WiFi configuration...");
    if (ssid != "") {
        setupWifi(ssid.c_str(), pass.c_str(), hostname.c_str());
    } else {
        defaultNoWifiHandler();
    }

    startOTA();

    xTaskCreatePinnedToCore(
            loopHandler, /* Task function. */
            "Serial/Firmware", /* name of task. */
            10000, /* Stack size of task */
            nullptr, /* parameter of the task */
            1, /* priority of the task */
            &baseAppTask, /* Task handle to keep track of created task */
            1); /* pin task to core 0 */
}



