#ifndef DEVICE_H
#define DEVICE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <libudev.h>

enum class DeviceType { Keyboard, Mouse };

class Device {
public:
    explicit Device(::udev* udev, uint16_t vid, uint16_t pid, DeviceType type);

    Device(Device&& other) = delete;
    Device& operator=(Device&&) = delete;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    uint16_t vid() const { return m_vid; }
    uint16_t pid() const { return m_pid; }
    std::string_view name() const;
    std::string_view devpath() const;
    std::string_view devnode() const;
    std::string_view typestr() const { return m_typestr[static_cast<int>(m_type)]; }
    DeviceType type() const { return m_type; }

    explicit operator bool() const { return m_udev_dev.get() != nullptr; }

private:
    ::udev_device* udev_parent() const;

    uint16_t m_vid;
    uint16_t m_pid;
    DeviceType m_type;
    std::unique_ptr<::udev_device, decltype(&::udev_device_unref)> m_udev_dev{nullptr, &::udev_device_unref};
    static constexpr std::string_view m_typestr[2]{"keyboard", "mouse"};
    static constexpr std::string_view m_unknown{"<unknown>"};
};





#endif /* #ifndef DEVICE_H */
