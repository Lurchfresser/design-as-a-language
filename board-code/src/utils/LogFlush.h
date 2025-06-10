// LogFlush.h — header‐only helper to auto‐flush Logger every 50 ms
#pragma once

#include <vector>
#include "utils/TaskRunner.h"
#include "utils/Logger.h"
#include "config.h"

namespace LogFlush {
    // “Token” instance for our Task<int>
    inline int _token = 0;
    // Task<Dummy> expects a vector of pointers; we give it a single dummy address
    inline std::vector<int*> _inst{ &_token };

    // Returns the singleton Task<int> that drives Logger::flushToSerial()
    inline Task<int>& task() {
        static Task<int> _task(
            "LogFlush",     
            _inst,          
            LOG_FLUSH_TASK_FREQ_HZ,             
            false,          
            [](int*) {      
                Logger::flushToSerial();
            },
            LOG_FLUSH_TASK_STACK_SIZE,           
            LOG_FLUSH_TASK_PRIORITY,              
            LOG_FLUSH_TASK_CORE               
        );
        return _task;
    }

    // Starts (or resumes) the flush task.
    inline void start() {
        task().start();
    }

    // (Optional) Stop flushing.
    inline void stop() {
        task().stop();
    }
}
