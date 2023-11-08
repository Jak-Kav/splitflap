#include "WebServer.h"
#include "webserver_task.h"
#include <functional>
#include <NimBLEDevice.h>

WebServer server(80);
#define SERVICE_UUID        "d8a2acc6-9a23-4813-9a2d-258508739882"
#define CHARACTERISTIC_UUID "20866fa9-1e90-4fca-8611-7682e47b8075"

WebServerTask::WebServerTask(SplitflapTask& splitflap_task, Logger& logger, const uint8_t task_core) :
        Task("WebServer", 8192, 1, task_core),
        splitflap_task_(splitflap_task),
        logger_(logger) {
            lastMessageDisplayTime = millis() - messageDisplayCooldown;
}

String sendHTML() { 
    String html = "<html><body>";
    html += "<h1>Splitflap Display</h1>";
    html += "<p>Enter a message (up to 6 characters):</p>";
    html += "<form method='get' action='/display'>";
    html += "<input type='text' name='message' maxlength='6' placeholder='Message'>";
    html += "<input type='submit' value='Display Message'>";
    html += "</form>";
    html += "<p>Click the button to recalibrate:</p>";
    html += "<form method='get' action='/reset'>";
    html += "<input type='text' name='reset_pass' maxlength='6' placeholder='Reset_Pass'>";
    html += "<input type='submit' value='Reset'>";
    html += "</form>";
    html += "</body></html>";
    return html;
}

void WebServerTask::handle_root() {
  server.send(200, "text/html", sendHTML());
}

void WebServerTask::handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void WebServerTask::handle_display() {
    unsigned long currentTime = millis();

    if (server.hasArg("message")) {
        if (currentTime - lastMessageDisplayTime >= messageDisplayCooldown) {
            String message = server.arg("message");
            message.toLowerCase();
            splitflap_task_.showString(message.c_str(), NUM_MODULES, false);
            
            // Set the message display state
            displayingMessage = true;
            messageDisplayStartTime = currentTime;
            lastMessageDisplayTime = currentTime;

            // Send an HTTP response
            server.send(200, "text/html", sendHTML());
        } else {
            server.send(400, "text/plain", "Message only allowed every 10 mins!");
        }
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void WebServerTask::handle_reset() {
    if (server.hasArg("reset_pass")) {
        String reset_pass = server.arg("reset_pass");
        if (reset_pass == "reset") {
            splitflap_task_.resetAll();
            server.send(200, "text/html", sendHTML());
        } else {
            server.send(400, "text/plain", "Wrong Password!");
        }
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

class WiFiIPCallback : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) {
        IPAddress localIP = WiFi.localIP();
        std::string ip_string = localIP.toString().c_str();
        pCharacteristic->setValue(ip_string);
    }
};

void WebServerTask::setup_bluetooth() {
    BLEDevice::init("Splitty McSplitflap");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID,
                                    /***** Enum Type NIMBLE_PROPERTY now *****      
                                            BLECharacteristic::PROPERTY_READ   |
                                            BLECharacteristic::PROPERTY_WRITE  
                                            );
                                    *****************************************/
                                            NIMBLE_PROPERTY::READ
                                        );
    pCharacteristic->setCallbacks(new WiFiIPCallback());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);

    BLEDevice::startAdvertising();
}

void WebServerTask::run() {
    server.on("/", std::bind(&WebServerTask::handle_root, this));
    server.on("/display", std::bind(&WebServerTask::handle_display, this));
    server.on("/reset", std::bind(&WebServerTask::handle_reset, this));
    server.onNotFound(std::bind(&WebServerTask::handle_NotFound, this));
    server.begin();

    setup_bluetooth();

    while(1) {
        server.handleClient();
        delay(1000);
    }
}
