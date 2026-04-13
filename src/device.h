#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <libudev.h>

class InputInterface {
public:
    enum Type { Keyboard, Mouse };

    explicit InputInterface(::udev_device* udev_dev, Type type);

    InputInterface(InputInterface&& other);
    InputInterface& operator=(InputInterface&&) = delete;
    InputInterface(const InputInterface&) = delete;
    InputInterface& operator=(const InputInterface&) = delete;

    std::string_view name() const;
    std::string_view devpath() const;
    std::string_view devnode() const;
    std::string_view typestr() const { return m_typestr[m_type]; }
    Type type() const { return m_type; }

private:
    std::unique_ptr<::udev_device, decltype(&::udev_device_unref)> m_udev_dev;
    Type m_type;
    static constexpr std::string_view m_typestr[2]{"keyboard", "mouse"};
    static constexpr std::string_view m_unknown{"<unknown>"};

    ::udev_device* udev_parent() const;
};

class Device {
public:
    explicit Device(::udev* udev, const char* vid, const char* pid);

    Device(Device&& other) = delete;
    Device& operator=(Device&&) = delete;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    std::string_view vid() const { return m_vid; }
    std::string_view pid() const { return m_pid; }
    const std::vector<InputInterface>& input_interfaces() const { return m_input_interfaces; }

private:
    std::string m_vid;
    std::string m_pid;
    std::vector<InputInterface> m_input_interfaces;
};





#endif /* #ifndef DEVICE_H */
