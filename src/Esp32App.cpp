//
// Created by Brian Demers on 10/4/20.
//

#include "Esp32App.h"
#include <WiFi.h>
#include <Preferences.h>

static SimpleCLI simpleCli;

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

void registerCommands() {
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

    // help
    simpleCli.addCommand("help", helpCommandCallback)
        .setDescription("Prints this help message");
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
        Serial.println("WiFi not configured use 'wifi -ssid <your-ssid> -pass <your-pass>'");
    }
}

String input;
void Esp32App::handleSerialInput() {
    // Check if user typed something into the serial monitor
    if (Serial.available()) {
        char c = Serial.read();

        if (c == '\b') {
            input = input.substring(0, input.length() - 1);
        } else {
            input += c;
        }

        if (c == '\r') {
            simpleCli.parse(input);
            input = "";
        } else {
            Serial.print(c);
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



