#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <functional>

#include "config.h"
#include "utils/Logger.h"
#include "utils/TaskRunner.h"

/**
 * Possible states of the Wi‑Fi controller.
 */
enum class WifiState : uint8_t {
    DISCONNECTED,   ///< Station mode: not connected to an AP.
    CONNECTING,     ///< STA is attempting to connect / reconnect.
    CONNECTED,      ///< STA is connected and has an IP address.
    ACCESS_POINT    ///< Device is running in Soft‑AP mode.
};

/**
 * @brief Simple Wi‑Fi management wrapper for ESP32 (Arduino framework).
 *
 *  * Periodically checks the current connection state in a background task
 *    (frequency defined via `WIFI_TASK_FREQ_HZ` in *config.h*).
 *  * Automatically tries to reconnect after the interval defined in
 *    `WIFI_RECONNECT_INTERVAL_MS` (also in *config.h*).
 *  * Exposes `onWifiStateChange()` allowing the user to register a
 *    callback that receives the new `WifiState` whenever it changes.
 *
 */
class WifiController {
private:
    String ssid;                    ///< Target SSID in STA mode.
    String password;                ///< Target password in STA mode.

    WifiState wifiState;            ///< Current state.
    std::function<void(WifiState)> stateChangeCallback;

    unsigned long lastConnectAttempt; ///< Timestamp of last Wi‑Fi.begin / reconnect.

    Task<WifiController>* wifiControllerTask; ///< Periodic task pointer.

    /**
     * @brief Internal helper: update state & invoke callback if it changed.
     */
    inline void setState(WifiState newState) {
        if (wifiState != newState) {
            wifiState = newState;
            if (stateChangeCallback) {
                stateChangeCallback(wifiState);
            }
        }
    }

public:
    /**
     * @param ssid      Optional SSID to auto‑connect to in STA mode.
     * @param password  Optional passphrase for the SSID.
     */
    WifiController(const String& ssid = "", const String& password = "")
        : ssid(ssid),
          password(password),
          wifiState(WifiState::DISCONNECTED),
          stateChangeCallback(nullptr),
          lastConnectAttempt(0),
          wifiControllerTask(nullptr) {}

    /**
     * @brief Starts Wi‑Fi (STA if an SSID is supplied) and schedules the
     *        background task.
     */
    void begin() {
        if (ssid.length()) {
            connectToWiFi(ssid, password);
        }

        wifiControllerTask = new Task<WifiController>(
            "WifiControllerTask",         // Task name
            { this },                      // Captured object
            WIFI_TASK_FREQ_HZ,             // Frequency (Hz)
            false,                         // Run on fixed schedule
            [](WifiController* wc) { wc->updateState(); },
            WIFI_TASK_STACK_SIZE,                          // Stack bytes
            WIFI_TASK_PRIORITY,                             // Priority
            WIFI_TASK_CORE                 // CPU core
        );
        wifiControllerTask->start();
    }

    /** Connection helpers ***********************************************************/

    /**
     * @brief Switch to STA mode and connect / reconnect.
     */
    void connectToWiFi(const String& _ssid, const String& _password) {
        ssid = _ssid;
        password = _password;

        Logger::logInfo("Connecting to SSID " + ssid, "WiFiStatus");
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        setState(WifiState::CONNECTING);
        lastConnectAttempt = millis();
    }

    /**
     * @brief Disconnect from the current network and stop STA mode.
     */
    void disconnect() {
        WiFi.disconnect(true);
        setState(WifiState::DISCONNECTED);
    }

    /**
     * @brief Start / switch to Soft‑AP mode.
     * @param apSsid     SSID for the Soft‑AP.
     * @param apPassword Passphrase (≥8 chars) or empty string for open AP.
     * @param channel    Wi‑Fi channel (1‑13).
     * @param hidden     When true the AP will not broadcast its SSID.
     */
    void startAP(const String& apSsid, const String& apPassword,
                 uint8_t channel = 1, bool hidden = false) {
        Logger::logInfo("Starting Access Point %s", apSsid.c_str());
        WiFi.mode(WIFI_AP);
        WiFi.softAP(apSsid.c_str(), apPassword.c_str(), channel, hidden);
        setState(WifiState::ACCESS_POINT);
    }

    /*********************************************************************************/

    /**
     * @brief Register a callback that is executed every time the state changes.
     * @param callback  `void(WifiState)` style callback.
     */
    void onWifiStateChange(const std::function<void(WifiState)>& callback) {
        stateChangeCallback = callback;
    }

    /** @return Current Wi‑Fi state. */
    inline WifiState getState() const { return wifiState; }

    /**
     * @brief Task callback – polls ESP32 Wi‑Fi stack & drives reconnection.
     */
    void updateState() {
        if (WiFi.getMode() == WIFI_STA) {
            wl_status_t status = WiFi.status();
            switch (status) {
                case WL_CONNECTED:
                    setState(WifiState::CONNECTED);
                    break;

                case WL_DISCONNECTED:
                case WL_CONNECTION_LOST:
                case WL_NO_SSID_AVAIL:
                case WL_CONNECT_FAILED:
                    setState(WifiState::DISCONNECTED);
                    // Throttle reconnect attempts.
                    if (millis() - lastConnectAttempt >= WIFI_RECONNECT_INTERVAL_MS && ssid.length()) {
                        Logger::logInfo("Reconnecting to SSID " + ssid, "WiFiStatus");
                        WiFi.reconnect();
                        setState(WifiState::CONNECTING);
                        lastConnectAttempt = millis();
                    }
                    break;

                case WL_IDLE_STATUS:
                default:
                    setState(WifiState::CONNECTING);
                    break;
            }
        } else if (WiFi.getMode() == WIFI_AP) {
            // Remain in Soft‑AP mode until user explicitly changes mode.
            setState(WifiState::ACCESS_POINT);
        } else {
            setState(WifiState::DISCONNECTED);
        }
    }
};
