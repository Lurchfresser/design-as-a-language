#pragma once

#include <Arduino.h>
#include "utils/Logger.h"
#include <vector>
#include <functional>
#include <string>
#include <atomic>
#include "config.h"

/**
 * A templated helper that wraps a FreeRTOS task and exposes an explicit
 * lifecycle: start → pause/resume → stop.  The task can be started more than
 * once; after it finishes (graceful stop) a subsequent start() creates a fresh
 * FreeRTOS task.  Calling pause()/resume()/stop() before the first start()
 * only logs an error.
 */
template <typename T>
class Task
{
private:
    std::string taskName;
    std::vector<T *> instances;
    unsigned long interval; // milliseconds between iterations
    bool oneShot;
    std::function<void(T *)> callback;

    TaskHandle_t taskHandle = nullptr;
    std::atomic<bool> terminateRequested{false};
    std::atomic<bool> terminated{true}; // true ⇒ no running task
    std::atomic<bool> paused{false};

    uint32_t stackDepth; // words of stack
    int priority;
    int core;

    int hz2ms(unsigned long hz)
    {
        return (hz == 0) ? 0 : (1000 / hz);
    }

public:
    /**
     * Constructor.
     * @param taskName       Name for the task.
     * @param instances      Vector of pointers to T instances.
     * @param frequency      Task frequency in Hz.
     * @param oneShot        True if the task should only run once.
     * @param callback       Function to call for each instance.
     * @param stackDepth     Stack depth for the task (words).
     * @param priority       Task priority.
     * @param core           Core number (0 or 1).
     */
    Task(const std::string &taskName,
         const std::vector<T *> &instances,
         unsigned long frequency,
         bool oneShot,
         std::function<void(T *)> callback,
         uint32_t stackDepth,
         int priority,
         int core)
        : taskName(taskName),
          instances(instances),
          interval(hz2ms(frequency)),
          oneShot(oneShot),
          callback(callback),
          stackDepth(stackDepth),
          priority(priority),
          core(core)
    {
        if (frequency == 0 && !oneShot)
        {
            Logger::logError(
                String("Frequency must be >0 for periodic tasks (") + String(taskName.c_str()) + ")",
                "TaskInvalidFrequency");
        }
        if (core < 0 || core > 1)
        {
            Logger::logError(
                String("Invalid core specified for ") + String(taskName.c_str()),
                "TaskInvalidCoreError");
        }
    }

    /** Start or resume the task. */
    void start()
    {
        if (taskHandle == nullptr || terminated.load())
        {
            createTask();
            return;
        }
        if (paused.load())
        {
            resume();
            return;
        }
        Logger::logInfo(
            String("Task ") + String(taskName.c_str()) + " already running.",
            String(taskName.c_str()) + "TaskAlreadyRunning");
    }

    /** Pause the task execution. */
    void pause()
    {
        if (taskHandle == nullptr)
        {
            Logger::logError(
                String("Task ") + String(taskName.c_str()) + " not yet started. Cannot pause.",
                String(taskName.c_str()) + "TaskPauseError");
            return;
        }
        if (!paused.load())
        {
            vTaskSuspend(taskHandle);
            paused.store(true);
            if (LOG_TASK_STATUS)
            {
                Logger::logInfo(
                    String(taskName.c_str()) + " Task paused",
                    String(taskName.c_str()) + "TaskStatus");
            }
        }
    }

    /** Resume a paused task. */
    void resume()
    {
        if (taskHandle == nullptr)
        {
            Logger::logError(
                String("Task ") + String(taskName.c_str()) + " not yet started. Cannot resume.",
                "TaskResumeError");
            return;
        }
        if (paused.load())
        {
            vTaskResume(taskHandle);
            paused.store(false);
            if (LOG_TASK_STATUS)
            {
                Logger::logInfo(
                    String(taskName.c_str()) + " Task resumed",
                    String(taskName.c_str()) + "TaskStatus");
            }
        }
    }

    /** Request a graceful delete (stop). */
    void stop()
    {
        if (taskHandle == nullptr)
        {
            Logger::logError(
                String("Task ") + String(taskName.c_str()) + " not yet started. Cannot stop.",
                "TaskStopError");
            return;
        }
        requestGracefulDelete();
    }

    bool isStarted() const { return taskHandle != nullptr; }
    /** Get the underlying FreeRTOS task handle. */
    TaskHandle_t getHandle() const { return taskHandle; }
    /** Check if the task has terminated. */
    bool isTerminated() const { return terminated.load(); }

    /** Destructor ensures the task is stopped before the object is destroyed. */
    ~Task()
    {
        if (!terminated.load() && taskHandle)
        {
            requestGracefulDelete();
            while (!terminated.load())
            {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
    }

private:
    /** Internal: create the FreeRTOS task. */
    void createTask()
    {
        if (taskHandle != nullptr && !terminated.load())
        {
            Logger::logError(
                String("Task ") + String(taskName.c_str()) + " already running. Not creating again.",
                "TaskStartError");
            return;
        }

        terminateRequested.store(false);
        paused.store(false);
        terminated.store(false);

        BaseType_t result = xTaskCreatePinnedToCore(
            Task::taskRunner,
            taskName.c_str(),
            stackDepth,
            this,
            priority,
            &taskHandle,
            core);

        if (result != pdPASS)
        {
            terminated.store(true);
            taskHandle = nullptr;
            Logger::logError(
                String("Task creation failed for ") + String(taskName.c_str()),
                "TaskCreationError");
        }
        else
        {
            if (LOG_TASK_STATUS)
            {
                Logger::logInfo(
                    String(taskName.c_str()) + " Task created successfully",
                    String(taskName.c_str()) + "TaskStatus");
            }
        }
    }

    /** Internal: set the flag so runner exits. */
    void requestGracefulDelete()
    {
        if (terminated.load())
        {
            Logger::logError(
                String("Task ") + String(taskName.c_str()) + " already terminated. Cannot request deletion.",
                "TaskAlreadyTerminatedError");
            return;
        }
        if (paused.load())
        {
            resume();
        }
        terminateRequested.store(true);
    }

    /** The FreeRTOS task function. */
    static void taskRunner(void *pvParameters)
    {
        auto *self = static_cast<Task<T> *>(pvParameters);

        if (self->oneShot)
        {
            for (auto *inst : self->instances)
            {
                if (inst)
                    self->callback(inst);
            }
            self->terminated.store(true);
            self->taskHandle = nullptr;
            vTaskDelete(nullptr);
            return;
        }
        else
        {
            TickType_t lastWake = xTaskGetTickCount();
            const TickType_t period = pdMS_TO_TICKS(self->interval);
            while (!self->terminateRequested.load())
            {
                for (auto *inst : self->instances)
                {
                    if (inst)
                        self->callback(inst);
                }

                BaseType_t ok = xTaskDelayUntil(&lastWake, period);

                if (ok == pdFALSE)
                { // woke up late
                    // Were we just resumed?  Reset baseline and skip error.
                    // TODO rework this. what do i want to achieve here?
                    lastWake = xTaskGetTickCount();
                    continue;
                }
            }

            self->terminated.store(true);
            self->taskHandle = nullptr;
            vTaskDelete(nullptr);
        }
    }
};