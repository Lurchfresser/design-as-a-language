#include <Arduino.h>
#include "utils/Logger.h"
#include "utils/LogFlush.h"
#include "utils/TaskRunner.h"
#include "utils/WifiController.h"
#include "utils/MqttController.h"

const char *mqtt_server = "1acf1903347f41bbbe44290fb15abd87.s1.eu.hivemq.cloud"; // Mosquitto Server URL
const char *mqtt_user = "design-as-a-language";                   // MQTT username
const char *mqtt_password = "Test12345";

WifiController *wifiController = new WifiController(
    "CoCoLabor3_IoT", "cocolabor12345");
MqttController *mqttController = new MqttController(
    *wifiController, // Dereference the pointer
    String(mqtt_server),
    1883,
    String("ESP32-") + String(ESP.getEfuseMac(), 16),
    String(mqtt_user),
    String(mqtt_password));

void setup()
{
    Logger::begin();
    LogFlush::start();
    Serial.begin(115200);
    wifiController->begin();
    mqttController->begin();
    mqttController->onMqttStateChange([](MqttState state)
                                      { 
if (state == MqttState::CONNECTED) {
    mqttController->subscribe("test");
   } });

    mqttController->onMessage([](const char *topic, const uint8_t *payload, unsigned int length)
                              {
           char msg = 0;
          String message = "";
          for (int i = 0; i < length; i++) {
              message += (char)payload[i];
          }
           Logger::logInfo( topic,"topic"); 
           Serial.println(msg);
           Logger::logInfo(message,"msg"); });
}

void loop()
{
    // Main loop does nothing, all work is done in tasks
    delay(1000);
}
