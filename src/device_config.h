#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include "device.h"
#include "event_job.h"
#include <array>
#include <string>
#include <optional>
#include <unordered_map>

class VirtualDevice;

using EventJobMap = std::unordered_map<uint16_t, std::array<std::optional<uint32_t>, 3>>;

#define DEVICETYPE_KEYBOARD (1 << 0)
#define DEVICETYPE_MOUSE    (1 << 1)

struct DeviceConfig {
    uint16_t vid{0};
    uint16_t pid{0};
    int32_t type{0};
    std::string name;
    EventJobMap mappings;
    VirtualDevice* vdev{nullptr};
};

struct DeviceRef {
    uint32_t id;
};

#endif /* #ifndef DEVICE_CONFIG_H */
