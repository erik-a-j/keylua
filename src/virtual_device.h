#ifndef VIRTUAL_DEVICE_H
#define VIRTUAL_DEVICE_H

#include <string>
#include <string_view>

struct libevdev;
struct libevdev_uinput;
struct input_event;

class VirtualDevice {
public:
    explicit VirtualDevice(const ::libevdev* src);

    VirtualDevice(VirtualDevice&& other) = delete;
    VirtualDevice& operator=(VirtualDevice&&) = delete;
    VirtualDevice(const VirtualDevice&) = delete;
    VirtualDevice& operator=(const VirtualDevice&) = delete;

    ~VirtualDevice();

    void close();

    bool emit(const ::input_event& ev);

    explicit operator bool() const { return m_errbuf.empty() && m_uinput_dev != nullptr; }
    std::string_view errmsg() const { return m_errbuf; }
private:
    ::libevdev_uinput* m_uinput_dev;
    std::string m_errbuf;
};

#endif /* #ifndef VIRTUAL_DEVICE_H */
