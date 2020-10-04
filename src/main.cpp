#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "AccessoryInfo.h"
#include "Settings.h"
#include "Lights.h"
#include "FakeLight.h"
#include <SimpleCLI.h>
#include <Preferences.h>

#include "Esp32App.h"

#define ONBOARD_LED  2
#define CONTROL_PIN 23

// setting PWM properties
const uint16_t freq = 5000;
const uint8_t ledChannel = 0;
const uint8_t resolution = 8;

const int port = 9123;

WebServer server(port);

AccessoryInfo info;
Lights lights;
Settings settings;

Esp32App app;
Preferences preferences;

typedef std::function<void(DynamicJsonDocument&)> JsonFunction;

void loadSettings() {

    uint8_t on = preferences.getUChar("light-0-on", 1);
    uint8_t temp = preferences.getUChar("light-0-temp", 100); // TODO this doesn't do anything
    uint8_t brightness = preferences.getUChar("light-0-bright", 100);
    Serial.flush();

    lights.lights[0].on = on;
    lights.lights[0].temperature = temp;
    lights.lights[0].brightness = brightness;

    // TODO this _should_ be separate, but they are coupled for now
    settings.powerOnBehavior = on;
    settings.powerOnTemperature = temp;
    settings.powerOnBrightness = brightness;

    Serial.print("Loaded settings - on: ");
    Serial.println(on);

    Serial.print("Loaded settings - temp: ");
    Serial.println(temp);

    Serial.print("Loaded settings - brightness: ");
    Serial.println(brightness);
}

void writeSettings() {
    preferences.putUChar("light-0-on", lights.lights[0].on);
    preferences.putUChar("light-0-temp", lights.lights[0].temperature);
    preferences.putUChar("light-0-bright", lights.lights[0].brightness);
}

void ledOn(uint8_t &brightness) {
    // brightness is a percentage, convert to 0-255
    int pwmValue = 255 * brightness / 100;

    Serial.print("Setting light PWM to: ");
    Serial.println(pwmValue);

    ledcWrite(ledChannel, pwmValue);
}

void lightOn(uint8_t brightness) {
    // board LED on
    digitalWrite(ONBOARD_LED, HIGH);
    // LED strip PWM
    ledOn(brightness);
}

void lightOff() {
    // both off
    digitalWrite(ONBOARD_LED, LOW);
    uint8_t off = 0;
    ledOn(off);
}

void lightsChanges(Light &light) {

    Serial.print("PowerOn: ");
    Serial.println(light.on);

    Serial.print("Temperature: ");
    Serial.println(light.temperature);

    Serial.print("Brightness: ");
    Serial.println(light.brightness);

    if (light.on == 1) {
        lightOn(light.brightness);
    } else {
        lightOff();
    }

    // update settings with current info
    settings.powerOnBehavior = light.on;
    settings.powerOnTemperature = light.temperature;
    settings.powerOnBrightness = light.brightness;

    // persist the changes
    writeSettings();
}

void sendJson(DynamicJsonDocument &doc) {
    String buf;
    serializeJson(doc, buf);
    server.send(200, "application/json", buf);
}

void jsonError() {
    server.send(400, "application/json", R"({"error": "Failed to parse JSON body"})");
}

void parseJson(String body, const JsonFunction &handleJson, int size = 200) {
    DynamicJsonDocument doc(size);
    DeserializationError error = deserializeJson(doc, body);

    // Test if parsing succeeds.
    error ? jsonError()
          : handleJson(doc);
}

void getAccessoryInfo() {
    DynamicJsonDocument doc = info.toJson();
    sendJson(doc);
}

void postAccessoryInfo() {
    String body = server.arg("plain");
    parseJson(body, [](DynamicJsonDocument &doc) {
            info.fromJson(doc);
            preferences.putString("displayName", info.displayName);
            getAccessoryInfo();
    });
}

void getSettings() {
    DynamicJsonDocument doc = settings.toJson();
    sendJson(doc);
}

void putSettings() {
    String body = server.arg("plain");
    parseJson(body, [](DynamicJsonDocument &doc) {
        settings.fromJson(doc);
    });
    getSettings();
}

void getLights() {
    DynamicJsonDocument doc = lights.toJson();
    sendJson(doc);
}

void putLights() {
    String body = server.arg("plain");
    parseJson(body, [](DynamicJsonDocument &doc) {

        // parse lights
        lights.fromJson(doc);

        // handle the lights changed
        lightsChanges(lights.lights[0]);

        // return the GET lights json
        getLights();
    });
}

void identify() {
    // on off on off
    lightOn(100);
    delay(500);

    lightOff();
    delay(500);

    lightOn(100);
    delay(500);

    lightOff();
    delay(500);
}

// Define routing
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
                    F("Welcome to the REST Web Server"));
    });

    server.on(F("/elgato/accessory-info"), HTTP_GET, getAccessoryInfo);
    server.on(F("/elgato/accessory-info"), HTTP_PUT, postAccessoryInfo);
    server.on(F("/elgato/lights/settings"), HTTP_GET, getSettings);
    server.on(F("/elgato/lights/settings"), HTTP_PUT, putSettings);
    server.on(F("/elgato/lights"), HTTP_GET, getLights);
    server.on(F("/elgato/lights"), HTTP_PUT, putLights);
    server.on(F("/elgato/identify"), HTTP_POST, identify);
}

// Manage not found URL
void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void lightOnCommandCallback(cmd* c) {
    Command cmd(c);
    String on = cmd.getArg("on").getValue();
    lights.lights[0].on = on.toInt();

    lightsChanges(lights.lights[0]);
}

void lightTempCommandCallback(cmd* c) {
    Command cmd(c);
    String temp = cmd.getArg("temp").getValue();
    lights.lights[0].temperature = temp.toInt();

    lightsChanges(lights.lights[0]);
}

void lightBrightnessCommandCallback(cmd* c) {
    Command cmd(c);
    String brightness = cmd.getArg("brightness").getValue();
    lights.lights[0].brightness = brightness.toInt();

    lightsChanges(lights.lights[0]);
}

void mDnsCommandCallback(cmd* c) {
    Command cmd(c);
    String serviceName = cmd.getArg("service_name").getValue();
    String deviceId = cmd.getArg("device_id").getValue();

    preferences.begin("fake-light", false);
    preferences.putString("service_name", serviceName);
    preferences.putString("device_id", deviceId);
    preferences.end();

    ESP.restart();
}

void setupLEDs() {
    pinMode(ONBOARD_LED, OUTPUT);

    // configure PWM on CONTROL_PIN
    ledcSetup(ledChannel, freq, resolution);

    // attach the channel to the GPIO to be controlled
    ledcAttachPin(CONTROL_PIN, ledChannel);

    // set the initial state of LEDs
    lightsChanges(lights.lights[0]);
}

void setupMDNS(const char* serviceName, const char* deviceId) {
    // Activate mDNS this is used to be able to connect to the server
    if (MDNS.begin(WiFi.getHostname())) {
        Serial.println("MDNS responder started");
    }

    String deviceIdValue =  "id=";
    deviceIdValue.concat(deviceId);

    MDNS.setInstanceName(serviceName);
    MDNS.addService("elg", "tcp", port);
    MDNS.addServiceTxt("elg", "tcp", "mf", "Elgato");
    MDNS.addServiceTxt("elg", "tcp", "dt", "200");
    MDNS.addServiceTxt("elg", "tcp", "id", deviceIdValue);
    MDNS.addServiceTxt("elg", "tcp", "md", "Elgato Key Light Air 20LAB9901");
    MDNS.addServiceTxt("elg", "tcp", "pv", "1.0");
}

void setupHTTP() {
    // Set server routing
    restServerRouting();
    // Set not found response
    server.onNotFound(handleNotFound);
    // Start server
    server.begin();
    Serial.println("HTTP server started");
}

void registerCliCommands() {

    Command onCommand = app.addCommand("light-on", lightOnCommandCallback);
    onCommand.setDescription("Enables or disables light");
    onCommand.addPositionalArgument("on", "1");

    Command tempCommand = app.addCommand("light-temperature", lightTempCommandCallback);
    tempCommand.setDescription("Not Implemented");
    tempCommand.addPositionalArgument("temp", "1");

    Command brightCommand = app.addCommand("light-brightness", lightBrightnessCommandCallback);
    brightCommand.setDescription("Sets light brightness as a percentage 0-100");
    brightCommand.addPositionalArgument("brightness", "1");

    Command mdnsCommand = app.addCommand("mdns", mDnsCommandCallback);
    brightCommand.setDescription("Sets the mDNS service name and device id, and restarts device");
    brightCommand.addArg("service_name", DEFAULT_SERVICE_NAME);
    brightCommand.addArg("device_id", DEFAULT_DEVICE_ID);
}

bool serverStarted = false;
void setup() {
    // enable serial
    Serial.begin(9600);

    preferences.begin("fake-light", false);
    String serviceName = preferences.getString("service_name", DEFAULT_SERVICE_NAME);
    String deviceId = preferences.getString("device_id", DEFAULT_DEVICE_ID);
    info.displayName = preferences.getString("displayName", DEFAULT_DISPLAY_NAME);
    Serial.flush();

    registerCliCommands();
    app.begin();

    if (WiFi.status() == WL_CONNECTED) {

        // get the configuration
        loadSettings();

        // enable LEDs
        setupLEDs();

        // setup http server
        setupHTTP();

        // after everything is configured broadcast
        setupMDNS(serviceName.c_str(), deviceId.c_str());

        // mark the server started
        serverStarted = true;
    }
}

void loop() {
    app.handleSerialInput();

    if (serverStarted) {
        server.handleClient();
    }
}