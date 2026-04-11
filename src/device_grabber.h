#ifndef DEVICE_GRABBER_H
#define DEVICE_GRABBER_H

#include <fcntl.h>
#include <unistd.h>
#include <libevdev/libevdev.h>
#include <string>
#include <stdexcept>
#include <memory>
#include <cstring>

class DeviceGrabber {
    int m_fd{-1};
    libevdev* m_dev{nullptr};
public:
    using ptr = std::unique_ptr<DeviceGrabber>;

    DeviceGrabber(const std::string& path) {
        m_fd = open(path.c_str(), O_RDONLY|O_NONBLOCK);
        if (m_fd == -1) {
            throw std::runtime_error("Error: " + path + "does not exist");
        }
        int err = libevdev_new_from_fd(m_fd, &m_dev);
        if (err != 0) {
            throw std::runtime_error("libevdev_new_from_fd Error: " + std::string{std::strerror(err)});
        }
        err = libevdev_grab(m_dev, LIBEVDEV_GRAB);
        if (err != 0) {
            throw std::runtime_error("libevdev_grab Error: " + std::string{std::strerror(err)});
        }
    }

    ~DeviceGrabber() {
        libevdev_grab(m_dev, LIBEVDEV_UNGRAB);
        libevdev_free(m_dev);
        close(m_fd);
    }

    int fd() const { return m_fd; }
    const libevdev* dev() const { return m_dev; }
};

#endif /* #ifndef DEVICE_GRABBER_H */
