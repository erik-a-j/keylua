#ifndef DEVICE_H
#define DEVICE_H

#include <cstdint>
#include <string>
#include "raii.h"

struct device_info_t {
    std::string path;
    raii::libevdev dev;
    uint16_t product_id;
    uint16_t vendor_id;

    device_info_t(const char* path_, int product_id_, int vendor_id_, libevdev* dev_ = nullptr)
        : path{path_},
        dev{dev_},
        product_id{static_cast<uint16_t>(product_id_)},
        vendor_id{static_cast<uint16_t>(vendor_id_)} {}
};

#endif /* #ifndef DEVICE_H */
