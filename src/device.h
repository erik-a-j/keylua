#ifndef DEVICE_H
#define DEVICE_H

#include <unistd.h>
#include <cstdint>
#include <string>
#include <libevdev/libevdev.h>

template <bool OwnDevice>
class BaseDeviceInfo {
    std::string m_path;
    uint16_t m_product_id;
    uint16_t m_vendor_id;
    int m_fd;
    libevdev* m_dev;

public:
    explicit BaseDeviceInfo(const char* path, libevdev* dev, int fd)
        : m_path{path},
        m_product_id{static_cast<uint16_t>(::libevdev_get_id_product(dev))},
        m_vendor_id{static_cast<uint16_t>(::libevdev_get_id_vendor(dev))} {

        if constexpr (OwnDevice) {
            m_fd = fd;
            m_dev = dev;
        }
        else {
            m_fd = -1;
            m_dev = nullptr;
        }
    }

    ~BaseDeviceInfo() {
        if constexpr (OwnDevice) {
            (void)::libevdev_free(m_dev);
            (void)::close(m_fd);
        }
    }

    const std::string& path() const { return m_path; }
    uint16_t productID() const { return m_product_id; }
    uint16_t vendorID() const { return m_vendor_id; }
    int fd() const requires OwnDevice { return m_fd; }
    libevdev* dev() requires OwnDevice { return m_dev; }
};

using DeviceInfo = BaseDeviceInfo<false>;
using OwningDeviceInfo = BaseDeviceInfo<true>;


#endif /* #ifndef DEVICE_H */
