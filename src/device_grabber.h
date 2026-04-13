#ifndef DEVICE_GRABBER_H
#define DEVICE_GRABBER_H

#include <string>
#include <string_view>

struct libevdev;

class DeviceGrabber {
public:
    explicit DeviceGrabber(std::string_view devnode);

    DeviceGrabber(DeviceGrabber&& other);
    DeviceGrabber& operator=(DeviceGrabber&&) = delete;
    DeviceGrabber(const DeviceGrabber&) = delete;
    DeviceGrabber& operator=(const DeviceGrabber&) = delete;

    ~DeviceGrabber();

    void close();

    int fd() const { return m_fd; }
    const ::libevdev* dev() const { return m_dev; }

    explicit operator bool() const { return m_errbuf.empty() && m_fd != -1 && m_dev != nullptr; }
    std::string_view errmsg() const { return m_errbuf; }

private:
    int m_fd;
    ::libevdev* m_dev;
    std::string m_errbuf;
};

#endif /* #ifndef DEVICE_GRABBER_H */
