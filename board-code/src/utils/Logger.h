// Logger.h  — queue-only, RTOS-agnostic
#pragma once

#include "config.h"
#include <Arduino.h>
#include <cppQueue.h>

//--------------------------------------------------
// Common typedefs / message structs
//--------------------------------------------------
using millis_t = unsigned long;

struct ValueMessage {  String variableName; float value;  millis_t ts; };
struct ErrorMessage {  String errorMsg;     String name;  millis_t ts; };
struct InfoMessage  {  String infoMsg;      String name;  millis_t ts; };

class Logger {
public:
    //------------------  public API  ------------------

    static void begin() {
        pinMode(LED_ERROR, OUTPUT);
        digitalWrite(LED_ERROR, LOW);
    }

    static void logInfo (const String &msg, const String &name) {
        enqueue(infoQueue , new InfoMessage { msg,  name, millis() },
                "LoggerInfoQueueFull");
    }
    static void logValue(const String &var, float v) {
        enqueue(valueQueue, new ValueMessage{ var,  v,    millis() },
                "LoggerValueQueueFull");
    }
    static void logError(const String &err, const String &name) {
        enqueue(errorQueue, new ErrorMessage{ err,  name, millis() },
                "LoggerErrorQueueFull");
        digitalWrite(LED_ERROR, HIGH);
    }

    /** Flush everything that is queued right now. */
    static void flushToSerial() {
        drain<ValueMessage>(valueQueue , valueFmt );
        drain<ErrorMessage>(errorQueue , errorFmt );
        drain<InfoMessage >(infoQueue  , infoFmt  );
    }

private:
    //------------------  queues  ------------------
    inline static uint8_t valueBuf [sizeof(ValueMessage*) * LOGGER_VALUE_QUEUE_CAPACITY];
    inline static uint8_t errorBuf [sizeof(ErrorMessage*) * LOGGER_ERROR_QUEUE_CAPACITY];
    inline static uint8_t infoBuf  [sizeof(InfoMessage*)  * LOGGER_INFO_QUEUE_CAPACITY];

    inline static cppQueue valueQueue { sizeof(ValueMessage*), LOGGER_VALUE_QUEUE_CAPACITY, FIFO, false, valueBuf, sizeof(valueBuf) };
    inline static cppQueue errorQueue { sizeof(ErrorMessage*), LOGGER_ERROR_QUEUE_CAPACITY, FIFO, false, errorBuf, sizeof(errorBuf) };
    inline static cppQueue infoQueue  { sizeof(InfoMessage* ), LOGGER_INFO_QUEUE_CAPACITY , FIFO, false, infoBuf , sizeof(infoBuf ) };

    //------------------  helpers  ------------------
    template<class T>
    static void enqueue(cppQueue &q, T *ptr, const char *overflowTag) {
        if (!q.push(&ptr)) {
            delete ptr;
            Serial.println(errorFmt({ "queue full, dropping!", overflowTag, millis() }));
        }
    }

    template<class T>
    static void drain(cppQueue &q, String (*fmt)(const T&)) {
        T *ptr;
        while (q.pop(&ptr)) {
            Serial.println(fmt(*ptr));
            delete ptr;
        }
    }

    // formatters
    static String errorFmt(const ErrorMessage &m) { return ">error_" + m.name + ":"  + String(m.ts) + ":" + m.errorMsg   + "§Error|t"; }
    static String infoFmt (const InfoMessage  &m) { return ">info_"  + m.name + ":"  + String(m.ts) + ":" + m.infoMsg    + "§Info|t"; }
    static String valueFmt(const ValueMessage &m) { return ">"       + m.variableName + ":" + String(m.ts) + ":" + String(m.value) + "|g"; }
};
