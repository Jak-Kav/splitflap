#pragma once

#include <Arduino.h>
#include "WebServer.h"

#include "../core/logger.h"
#include "../core/splitflap_task.h"
#include "../core/task.h"

class WebServerTask : public Task<WebServerTask> {
    friend class Task<WebServerTask>; // Allow base Task to invoke protected run()

    public:
        WebServerTask(SplitflapTask& splitflap_task, Logger& logger, const uint8_t task_core);
        bool displayingMessage = false;
        unsigned long messageDisplayStartTime = 0;
        const unsigned long messageDisplayDuration = 5000;  // 5 seconds
        unsigned long lastMessageDisplayTime;
        const unsigned long messageDisplayCooldown = 10 * 60 * 1000;  // 10 minutes

    protected:
        void run();

    private:
        void handle_root();
        void handle_NotFound();
        void handle_display();
        void handle_reset();
        void setup_bluetooth();

        SplitflapTask& splitflap_task_;
        Logger& logger_;
};
