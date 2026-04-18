#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include "device.h"
#include "event_job.h"
#include <string>

class VirtualDevice;

struct DeviceConfig {
    uint16_t vid;
    uint16_t pid;
    DeviceType iface;
    std::string name;
    EventJobMap mappings;
    VirtualDevice* vdev{nullptr};
};

struct DeviceRef {
    uint32_t id;
};

#endif /* #ifndef DEVICE_CONFIG_H */
