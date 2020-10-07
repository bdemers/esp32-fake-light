//
// Created by Brian Demers on 10/6/20.
//

/*
 * Adapted from AsyncCallbackJsonWebHandler, in AsyncJson.h.  The only difference is that this implementation allows
 * any content type, or more specifically an unset content type.  The Elgato Control Center app does NOT set a
 * Content-Type header.
 */

#ifndef ESP32_LIGHT_JSONCALLBACKHANDLER_H
#define ESP32_LIGHT_JSONCALLBACKHANDLER_H

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <Print.h>

class JsonCallbackHandler: public AsyncWebHandler {
private:
protected:
    const String _uri;
    WebRequestMethodComposite _method;
    ArJsonRequestHandlerFunction _onRequest;
    size_t _contentLength;
    const size_t maxJsonBufferSize;
    size_t _maxContentLength;
public:

    JsonCallbackHandler(const String& uri, ArJsonRequestHandlerFunction onRequest, size_t maxJsonBufferSize=DYNAMIC_JSON_DOCUMENT_SIZE)
            : _uri(uri), _method(HTTP_POST|HTTP_PUT|HTTP_PATCH), _onRequest(onRequest), maxJsonBufferSize(maxJsonBufferSize), _maxContentLength(16384) {}
    void setMethod(WebRequestMethodComposite method){ _method = method; }
    void setMaxContentLength(int maxContentLength){ _maxContentLength = maxContentLength; }
    void onRequest(ArJsonRequestHandlerFunction fn){ _onRequest = fn; }

    virtual bool canHandle(AsyncWebServerRequest *request) override final{
        if(!_onRequest)
            return false;

        if(!(_method & request->method()))
            return false;

        if(_uri.length() && (_uri != request->url() && !request->url().startsWith(_uri+"/")))
            return false;

        request->addInterestingHeader("ANY");
        return true;
    }

    virtual void handleRequest(AsyncWebServerRequest *request) override final {
        if(_onRequest) {
            if (request->_tempObject != NULL) {
                DynamicJsonDocument jsonBuffer(this->maxJsonBufferSize);
                DeserializationError error = deserializeJson(jsonBuffer, (uint8_t*)(request->_tempObject));
                if(!error) {
                    JsonVariant json = jsonBuffer.as<JsonVariant>();
                    _onRequest(request, json);
                    return;
                }
            }
            request->send(_contentLength > _maxContentLength ? 413 : 400);
        } else {
            request->send(500);
        }
    }
    virtual void handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) override final {
    }
    virtual void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override final {
        if (_onRequest) {
            _contentLength = total;
            if (total > 0 && request->_tempObject == NULL && total < _maxContentLength) {
                request->_tempObject = malloc(total);
            }
            if (request->_tempObject != NULL) {
                memcpy((uint8_t*)(request->_tempObject) + index, data, len);
            }
        }
    }
    virtual bool isRequestHandlerTrivial() override final {return _onRequest ? false : true;}
};

#endif //ESP32_LIGHT_JSONCALLBACKHANDLER_H
