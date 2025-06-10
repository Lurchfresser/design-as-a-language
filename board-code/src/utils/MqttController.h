#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <functional>

#include "config.h"
#include "utils/Logger.h"
#include "utils/TaskRunner.h"
#include "utils/WifiController.h"

/**
 * Possible states of the MQTT controller.
 */
enum class MqttState : uint8_t {
    DISCONNECTED,   ///< Not connected to broker.
    CONNECTING,     ///< Trying to connect / reconnect.
    CONNECTED       ///< Active session established.
};

/**
 * @brief Lightweight MQTT manager that builds on top of @ref WifiController
 *        and PubSubClient.
 *
 *  * Automatically (re)connects whenever Wi‑Fi is connected.
 *  * Runs a background task (frequency `MQTT_TASK_FREQ_HZ`) that keeps
 *    the connection alive and pumps PubSubClient.
 *  * Exposes callbacks for state changes and incoming messages.
 */
class MqttController {
private:
    WifiController& wifi;        ///< Reference to shared Wi‑Fi controller.

    WiFiClient wifiClient;       ///< TCP client for TLS / plain MQTT.
    PubSubClient mqtt;           ///< PubSubClient instance.

    String brokerHost;
    uint16_t brokerPort;
    String user;
    String password;
    String clientId;

    MqttState mqttState;
    std::function<void(MqttState)> stateChangeCallback;
    std::function<void(const char*, const uint8_t*, unsigned int)> msgCallback;

    unsigned long lastConnectAttempt;

    Task<MqttController>* mqttControllerTask;

    /** Update state and invoke callback if changed. */
    inline void setState(MqttState newState) {
        if (mqttState != newState) {
            mqttState = newState;
            if (stateChangeCallback) {
                stateChangeCallback(mqttState);
            }
        }
    }

    /** Attempt broker connection (non‑blocking wrapper). */
    void connectToBroker() {
        if (!mqtt.connected()) {
            Logger::logInfo("Connecting to " + brokerHost + " - " + brokerPort, "MQTTConnectStatus");
            mqtt.setServer(brokerHost.c_str(), brokerPort);
            setState(MqttState::CONNECTING);
            if (mqtt.connect(clientId.c_str(), user.length() ? user.c_str() : nullptr,
                                              password.length() ? password.c_str() : nullptr)) {
                Logger::logInfo("Connected, session present=" + mqtt.connected(), "MQTTConnectStatus");
                setState(MqttState::CONNECTED);
                // (Re)subscribe to topics here if desired
            } else {
                Logger::logInfo("Connect failed rc=" + mqtt.state(), "MQTTConnectStatus");
                setState(MqttState::DISCONNECTED);
            }
        }
    }

public:
    /**
     * @param wifi           Reference to an initialised WifiController.
     * @param host           MQTT broker hostname/IP.
     * @param port           Broker TCP port (default 1883/8883).
     * @param clientId       MQTT client‑ID (random if empty).
     * @param user           Username (optional).
     * @param password       Password (optional).
     */
    MqttController(WifiController& wifi,
                   const String& host,
                   uint16_t      port = 1883,
                   const String& clientId = "",
                   const String& user = "",
                   const String& password = "")
        : wifi(wifi),
          mqtt(wifiClient),
          brokerHost(host),
          brokerPort(port),
          user(user),
          password(password),
          clientId(clientId.length() ? clientId : "ESP32‑" + String(ESP.getEfuseMac(), 16)),
          mqttState(MqttState::DISCONNECTED),
          stateChangeCallback(nullptr),
          msgCallback(nullptr),
          lastConnectAttempt(0),
          mqttControllerTask(nullptr) {

        mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE);
        mqtt.setCallback([this](char* topic, uint8_t* payload, unsigned int len) {
            if (msgCallback) {
                msgCallback(topic, payload, len);
            }
        });
    }

    /** @brief Start background maintenance task. */
    void begin() {
        mqttControllerTask = new Task<MqttController>(
            "MqttControllerTask",
            { this },
            MQTT_TASK_FREQ_HZ,
            false,
            [](MqttController* mc) { mc->updateState(); },
            MQTT_TASK_STACK_SIZE,
            MQTT_TASK_PRIORITY,
            MQTT_TASK_CORE);
        mqttControllerTask->start();
    }

    /** Register state‑change listener. */
    void onMqttStateChange(const std::function<void(MqttState)>& cb) { stateChangeCallback = cb; }

    /** Register message handler (topic, payload, length). */
    void onMessage(const std::function<void(const char*, const uint8_t*, unsigned int)>& cb) { msgCallback = cb; }

    /** Publish helpers. */
    bool publish(const String& topic, const String& payload, bool retained = false) {
        if (mqtt.connected()) {
            return mqtt.publish(topic.c_str(), payload.c_str(), retained);
        }
        return false;
    }

    bool subscribe(const String& topic, uint8_t qos = 0) {
        return mqtt.connected() ? mqtt.subscribe(topic.c_str(), qos) : false;
    }

    bool unsubscribe(const String& topic) {
        return mqtt.connected() ? mqtt.unsubscribe(topic.c_str()) : false;
    }

    /** Force immediate disconnect. */
    void disconnect() {
        mqtt.disconnect();
        setState(MqttState::DISCONNECTED);
    }

    /** Current state. */
    inline MqttState getState() const { return mqttState; }

    /**
     * Background pump – maintains connection and processes packets.
     */
    void updateState() {
        // Only operate if Wi‑Fi is up.
        if (wifi.getState() == WifiState::CONNECTED) {
            if (!mqtt.connected()) {
                if (millis() - lastConnectAttempt >= MQTT_RECONNECT_INTERVAL_MS) {
                    lastConnectAttempt = millis();
                    connectToBroker();
                }
            } else {
                setState(MqttState::CONNECTED);
            }
            // Process incoming/outgoing packets.
            mqtt.loop();
        } else {
            // Ensure we report DISCONNECTED when Wi‑Fi drops.
            if (mqtt.connected()) {
                mqtt.disconnect();
            }
            setState(MqttState::DISCONNECTED);
        }
    }
};
