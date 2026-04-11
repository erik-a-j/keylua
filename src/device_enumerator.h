#ifndef DEVICE_ENUMERATOR_H
#define DEVICE_ENUMERATOR_H

#include <cstdint>
#include <vector>
#include <string>

struct device_info_t {
    uint16_t vendor_id;
    uint16_t product_id;
    std::string path;

    device_info_t(int vendor_id_, int product_id_, const char* path_)
        : vendor_id{static_cast<uint16_t>(vendor_id_)},
        product_id{static_cast<uint16_t>(product_id_)},
        path{path_} {}
};

std::vector<device_info_t> enumerate_devices();

#endif /* #ifndef DEVICE_ENUMERATOR_H */
