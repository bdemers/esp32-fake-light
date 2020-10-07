//
// Created by Brian Demers on 10/4/20.
//

#include "Esp32WebApp.h"

void Esp32WebApp::registerCommands() {
    Esp32App::registerCommands();
}

Esp32WebApp::Esp32WebApp(AsyncWebServer &server) : server(server) {

    server.onNotFound([](AsyncWebServerRequest *request) {
        String message = "File Not Found 404\n\n";
        message += "URI: ";
        message += request->url();
        message += "\nMethod: ";
        message += (request->method() == HTTP_GET) ? "GET" : "POST";
        message += "\nArguments: ";
        message += request->args();
        message += "\n";
        for (uint8_t i = 0; i < request->args(); i++) {
            message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
        }

        message += "\nHeaders: ";
        message += request->headers();
        message += "\n";
        for (uint8_t i = 0; i < request->headers(); i++) {
            message += " " + request->headerName(i) + ": " + request->header(i) + "\n";
        }

        Serial.println(404);
        Serial.println(message);
        request->send(404, "text/plain", message);
    });
}

void Esp32WebApp::begin() {
    Esp32App::begin();

    if (WiFi.status() == WL_CONNECTED) {

        server.begin();
        Serial.println("HTTP server started");
    }
}