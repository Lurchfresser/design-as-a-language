#pragma once

#define REVERSE_DIRECTIONS true // set to true if its the left hand

// finger pins
#define THUMB_NAME "thumb"
#define THUMB_POTI 1
#define THUMB_FORWARD 48
#define THUMB_BACKWARD 47
#define THUMB_POTI_DOWN_ON_FORWARD false
#define THUMB_FWD_DRIVES_DOWN  true

#define INDEX_NAME "index"
#define INDEX_POTI 2
#define INDEX_FORWARD 14
#define INDEX_BACKWARD 21
#define INDEX_POTI_DOWN_ON_FORWARD true
#define INDEX_FWD_DRIVES_DOWN  false

#define MIDDLE_NAME "middle"
#define MIDDLE_POTI 6
#define MIDDLE_FORWARD 13
#define MIDDLE_BACKWARD 12
#define MIDDLE_POTI_DOWN_ON_FORWARD true
#define MIDDLE_FWD_DRIVES_DOWN false

#define RING_NAME "ring"
#define RING_POTI 4
#define RING_FORWARD 10
#define RING_BACKWARD 11
#define RING_POTI_DOWN_ON_FORWARD false
#define RING_FWD_DRIVES_DOWN   true

#define PINKY_NAME "pinky"
#define PINKY_POTI 5
#define PINKY_FORWARD 9
#define PINKY_BACKWARD 3
#define PINKY_POTI_DOWN_ON_FORWARD false
#define PINKY_FWD_DRIVES_DOWN  true

#define MAX_FINGER_NAME_LENGTH 32

// led pins
#define LED_APP 18
#define LED_CALIBRATION 19
#define LED_ERROR 20

// button
#define CALIBRATION_BUTTON_PIN 39
#define APP_BUTTON_PIN 40
#define BUTTON_DEBOUNCE_MS 50
#define BUTTON_LONG_PRESS_THRESHOLD_MS 1500

// calibration settings
#define CALIBRATION_TOLERANCE 100
#define EEPROM_START 0
#define EEPROM_MAGIC 0xCA1B

// feedback settings
#define PWM_FREQUENCY 5000
#define ADC_RESOLUTION 12

#define SENSOR_KALMAN_MEASUREMENT_ERROR 12.0f
#define SENSOR_KALMAN_ESTIMATE_ERROR 30.0f
#define SENSOR_KALMAN_PROCESS_NOISE 0.1f

#define ENABLE_DITHERING false

#define ERROR_POSITIVE_WHEN_ABOVE true
#define K_P 1.4f
#define K_D 0.1f
#define K_I 0.01f
#define INTEGRAL_ANTI_WINDUP_LIMIT 25000
#define SCALE_FACTOR 8.0f
#define NORMAL_DEADZONE 30
#define DERIVATIVE_DEADZONE 0.05f
#define ACTIVATION_THRESHHOLD 40

// logger settings
#define LOGGER_VALUE_QUEUE_CAPACITY 100
#define LOGGER_ERROR_QUEUE_CAPACITY 40
#define LOGGER_INFO_QUEUE_CAPACITY 40
#define LOG_TASK_STATUS false

// wifi and co settings
#define WIFI_RECONNECT_INTERVAL_MS 1000
#define WIFI_SSID "CoCoLabor3_IoT"
#define WIFI_PASSWORD "cocolabor12345"

#define MQTT_RECONNECT_INTERVAL_MS 500
#define MQTT_BROKER_HOST "broker.hivemq.com"
#define MQTT_BROKER_PORT 1883

#define UDP_REMOTE_IP        IPAddress(224,0,1,187)  
#define UDP_REMOTE_PORT      5683
#define UDP_LOCAL_PORT       5683                      // usually 5683

// feedback app settings
#define FEEDBACK_ROLE_TOPIC "feedback/role"
#define FEEDBACK_DATA_TOPIC "feedback/data"
#define FEEDBACK_PUBLISH_INTERVAL_MS 50    // 20 Hz

// task settings
// task frequencies
#define TASK_FREQ_MAX_HZ 125

#define DITHER_TASK_FREQ_HZ 125
#define PID_TASK_FREQ_HZ 100

#define APP_TASK_FREQ_HZ 20

#define BUTTON_TASK_FREQ_HZ 30

#define CALIBRATION_TASK_FREQ_HZ 10

#define LOG_FLUSH_TASK_FREQ_HZ 20
#define DEBUGGING_TASK_FREQ_HZ 19 // results in prime number interval for coverage of all intervals

#define WIFI_TASK_FREQ_HZ 10
#define HTTP_TASK_FREQ_HZ 10
#define MQTT_TASK_FREQ_HZ 50

// task cores
#define DITHER_TASK_CORE 0
#define PID_TASK_CORE 0

#define APP_TASK_CORE 0

#define BUTTON_TASK_CORE 0

#define CALIBRATION_TASK_CORE 0

#define LOG_FLUSH_TASK_CORE 1

#define DEBUGGING_TASK_CORE 1

#define HTTP_TASK_CORE 1
#define MQTT_TASK_CORE 1
#define WIFI_TASK_CORE 1

// task stack sizes
#define DITHER_TASK_STACK_SIZE 4096
#define PID_TASK_STACK_SIZE 4096

#define APP_TASK_STACK_SIZE 4096

#define BUTTON_TASK_STACK_SIZE 4096

#define CALIBRATION_TASK_STACK_SIZE 4096

#define LOG_FLUSH_TASK_STACK_SIZE 4096

#define DEBUGGING_TASK_STACK_SIZE 4096

#define HTTP_TASK_STACK_SIZE 8192
#define MQTT_TASK_STACK_SIZE 8192
#define WIFI_TASK_STACK_SIZE 8192

// task priorities
#define DITHER_TASK_PRIORITY 5
#define PID_TASK_PRIORITY 5

#define APP_TASK_PRIORITY 3

#define BUTTON_TASK_PRIORITY 4

#define CALIBRATION_TASK_PRIORITY 3

#define LOG_FLUSH_TASK_PRIORITY 1

#define DEBUGGING_TASK_PRIORITY 1

#define HTTP_TASK_PRIORITY 4
#define MQTT_TASK_PRIORITY 4
#define WIFI_TASK_PRIORITY 4