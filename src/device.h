#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <libevdev/libevdev.h>
#include <libudev.h>

class EventNode {
public:
    enum Type { Keyboard, Mouse };

    explicit EventNode(::udev_device* udev_dev, Type type);

    EventNode(EventNode&& other);
    EventNode& operator=(EventNode&&) = delete;
    EventNode(const EventNode&) = delete;
    EventNode& operator=(const EventNode&) = delete;

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

    udev_device* udev_parent() const;
};

class Device {
public:
    explicit Device(udev* udev, const char* vid, const char* pid);

    Device(Device&& other) = delete;
    Device& operator=(Device&&) = delete;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    std::string_view vid() const { return m_vid; }
    std::string_view pid() const { return m_pid; }
    const std::vector<EventNode>& evnodes() const { return m_evnodes; }

private:
    std::string m_vid;
    std::string m_pid;
    std::vector<EventNode> m_evnodes;
};

class DeviceGrabber {
public:
    explicit DeviceGrabber(const EventNode& evnode);

    DeviceGrabber(DeviceGrabber&& other);
    DeviceGrabber& operator=(DeviceGrabber&&) = delete;
    DeviceGrabber(const DeviceGrabber&) = delete;
    DeviceGrabber& operator=(const DeviceGrabber&) = delete;

    ~DeviceGrabber();

    int fd() const { return m_fd; }
    const libevdev* dev() const { return m_dev; }

    explicit operator bool() const {
        return m_errbuf.empty() && m_fd != -1 && m_dev != nullptr;
    }
    std::string_view errmsg() const { return m_errbuf; }

private:
    int m_fd;
    libevdev* m_dev;
    const EventNode& m_evnode;
    std::string m_errbuf;
};

#endif /* #ifndef DEVICE_H */
