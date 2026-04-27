#ifndef MONITOR_H
#define MONITOR_H

#include "device_config.h"
#include "event_job.h"
#include <cstdint>
#include <vector>

struct input_event;

struct usr_data_t {
    const std::vector<DeviceConfig>& devices;
    const std::vector<EventJob>& jobs;
};

void monitor_event_callback(uint32_t device_id, const input_event* ev, void* usr_data);

#endif /* #ifndef MONITOR_H */
