#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include "JsonCallbackHandler.h"
#include "AccessoryInfo.h"
#include "Settings.h"
#include "Lights.h"
#include "FakeLight.h"
#include <SimpleCLI.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include "Esp32WebApp.h"

#define ONBOARD_LED  2
#define CONTROL_PIN 23

// setting PWM properties
const uint16_t freq = 5000;
const uint8_t ledChannel = 0;
const uint8_t resolution = 8;

const int port = 9123;
AsyncWebServer server(port);
AccessoryInfo info;
Lights lights;
Settings settings;
Esp32WebApp app(server);


typedef std::function<void(JsonObject &)> WriteJsonFunction;

void loadSettings() {

    Preferences preferences;
    preferences.begin("fake-light");
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
    Preferences preferences;
    preferences.begin("fake-light", false);
    preferences.putUChar("light-0-on", lights.lights[0].on);
    preferences.putUChar("light-0-temp", lights.lights[0].temperature);
    preferences.putUChar("light-0-bright", lights.lights[0].brightness);
    preferences.end();
}

void changeLight(bool on, uint8_t &brightness) {

    // board LED on
    digitalWrite(ONBOARD_LED, on ? HIGH : LOW);
    // LED strip PWM

    // brightness is a percentage, convert to 0-255
    int pwmValue = 255 * brightness / 100;

    Serial.print("Setting light PWM to: ");
    Serial.println(pwmValue);

    ledcWrite(ledChannel, pwmValue);
}

void lightOn() {
    changeLight(true, lights.lights[0].brightness);
}

void lightOff() {
    uint8_t off = 0;
    changeLight(false, off);
}

void lightsChanges(Light &light) {

    Serial.print("PowerOn: ");
    Serial.println(light.on);

    Serial.print("Temperature: ");
    Serial.println(light.temperature);

    Serial.print("Brightness: ");
    Serial.println(light.brightness);

    if (light.on == 1) {
        changeLight(true, light.brightness);
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

void initLEDs() {
    pinMode(ONBOARD_LED, OUTPUT);

    // configure PWM on CONTROL_PIN
    ledcSetup(ledChannel, freq, resolution);

    // attach the channel to the GPIO to be controlled
    ledcAttachPin(CONTROL_PIN, ledChannel);

    // set the initial state of LEDs
    lightsChanges(lights.lights[0]);
}

void sendJson(AsyncWebServerRequest *request, WriteJsonFunction jsonFunction) {
    auto * response = new AsyncJsonResponse();
    JsonObject jsonObject = response->getRoot();
    jsonFunction(jsonObject);
    response->setLength();
    request->send(response);
}

void getAccessoryInfo(AsyncWebServerRequest *request) {
    sendJson(request, [](JsonObject & jsonObject){
        info.toJson(jsonObject);
    });
}

void putAccessoryInfo(AsyncWebServerRequest *request, JsonVariant &json) {

    JsonObject jsonObj = json.as<JsonObject>();
    info.fromJson(jsonObj);

    Preferences preferences;
    preferences.begin("fake-light", false);
    preferences.putString("displayName", info.displayName);
    preferences.end();
    getAccessoryInfo(request);
}

void getSettings(AsyncWebServerRequest *request) {
    sendJson(request, [](JsonObject & jsonObject){
        settings.toJson(jsonObject);
    });
}

void putSettings(AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();
    settings.fromJson(jsonObj);
    getSettings(request);
}

void getLights(AsyncWebServerRequest *request) {
    sendJson(request, [](JsonObject & jsonObject){
        lights.toJson(jsonObject);
    });
}

void putLights(AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();
    lights.fromJson(jsonObj);

    // handle the lights changed
    lightsChanges(lights.lights[0]);

    // return the GET lights json
    getLights(request);
}

void identify(AsyncWebServerRequest * request) {
    // on off on off
    lightOn();
    delay(500);

    lightOff();
    delay(500);

    lightOn();
    delay(500);

    lightOff();
    delay(500);

    request->send(200);
}

void notFound(AsyncWebServerRequest * request) {
    request->send(404);
}

// Define routing
void restServerRouting() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(200, "text/html", "Welcome to the REST Web Server");
    });

    // GET - /elgato/lights
    server.on("/elgato/lights", HTTP_GET, getLights);
    // PUT - /elgato/lights
    auto* lightsHandler = new JsonCallbackHandler("/elgato/lights", putLights);
    lightsHandler->setMethod(HTTP_PUT);
    server.addHandler(lightsHandler);

    // GET - /elgato/accessory-info
    server.on("/elgato/accessory-info", HTTP_GET, getAccessoryInfo);
    // PUT - /elgato/accessory-info
    auto* accessoryHandler = new JsonCallbackHandler("/elgato/accessory-info", putAccessoryInfo);
    accessoryHandler->setMethod(HTTP_PUT);
    server.addHandler(accessoryHandler);

    // GET - elgato/lights/settings
    server.on("/elgato/lights/settings", HTTP_GET, getSettings);
    // PUT - elgato/lights/settings
    auto* settingsHandler = new JsonCallbackHandler("/elgato/lights/settings", putSettings);
    settingsHandler->setMethod(HTTP_PUT);
    server.addHandler(settingsHandler);

    // POST - /elgato/identify
    server.on("/elgato/identify", HTTP_POST, identify);

    // GET = /elgato/battery-info - force empty 404
    server.on("/elgato/battery-info", HTTP_GET, notFound);
}

void setupMDNS(const char* serviceName, const char* deviceId) {
    // Activate mDNS this is used to be able to connect to the server
    if (MDNS.begin(WiFi.getHostname())) {
        Serial.println("MDNS responder started");
    }

    MDNS.setInstanceName(serviceName);
    MDNS.addService("elg", "tcp", port);
    MDNS.addServiceTxt("elg", "tcp", "mf", "Elgato");
    MDNS.addServiceTxt("elg", "tcp", "dt", "200");
    MDNS.addServiceTxt("elg", "tcp", "id", deviceId);
    MDNS.addServiceTxt("elg", "tcp", "md", "Elgato Key Light Air 20LAB9901");
    MDNS.addServiceTxt("elg", "tcp", "pv", "1.0");

    Serial.print("\tService Name: ");
    Serial.println(serviceName);
    Serial.print("\tDevice ID: ");
    Serial.println(deviceId);
}

void registerCliCommands() {

    Command onCommand = app.addCommand("light-on", [](cmd *c) {
        Command cmd(c);
        String on = cmd.getArg("on").getValue();
        lights.lights[0].on = on.toInt();
        lightsChanges(lights.lights[0]);
    });
    onCommand.setDescription("Enables or disables light");
    onCommand.addPositionalArgument("on", "1");

    Command tempCommand = app.addCommand("light-temperature", [](cmd * c) {
        Command cmd(c);
        String temp = cmd.getArg("temp").getValue();
        lights.lights[0].temperature = temp.toInt();
        lightsChanges(lights.lights[0]);
    });
    tempCommand.setDescription("Not Implemented");
    tempCommand.addPositionalArgument("temp", "1");

    Command brightCommand = app.addCommand("light-brightness", [](cmd * c) {
        Command cmd(c);
        String brightness = cmd.getArg("brightness").getValue();
        lights.lights[0].brightness = brightness.toInt();
        lightsChanges(lights.lights[0]);
    });
    brightCommand.setDescription("Sets light brightness as a percentage 0-100");
    brightCommand.addPositionalArgument("brightness", "1");

    Command mdnsCommand = app.addCommand("mdns", [](cmd * c) {
        Command cmd(c);
        String serviceName = cmd.getArg("service_name").getValue();
        String deviceId = cmd.getArg("device_id").getValue();

        Preferences preferences;
        preferences.begin("fake-light", false);
        preferences.putString("service_name", serviceName);
        preferences.putString("device_id", deviceId);
        preferences.end();

        ESP.restart();
    });
    mdnsCommand.setDescription("Sets the mDNS service name and device id, and restarts device");
    mdnsCommand.addArg("service_name", DEFAULT_SERVICE_NAME);
    mdnsCommand.addArg("device_id", DEFAULT_DEVICE_ID);
}

void setup() {
    // enable serial
    Serial.begin(9600);

    Preferences preferences;
    preferences.begin("fake-light", false);
    String serviceName = preferences.getString("service_name", DEFAULT_SERVICE_NAME);
    String deviceId = preferences.getString("device_id", DEFAULT_DEVICE_ID);
    info.displayName = preferences.getString("displayName", DEFAULT_DISPLAY_NAME);
    preferences.end();
    Serial.flush(); // flush is required after getting preferences

    registerCliCommands();
    app.begin();

    if (WiFi.status() == WL_CONNECTED) {

        // get the initial configuration
        loadSettings();

        // enable LEDs
        initLEDs();

        // setup http server
        restServerRouting();

        // after everything is configured broadcast
        setupMDNS(serviceName.c_str(), deviceId.c_str());
    }
}

void loop() {}