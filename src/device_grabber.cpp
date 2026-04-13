
#include "device_grabber.h"
#include <libevdev/libevdev.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

DeviceGrabber::DeviceGrabber(std::string_view devnode)
    : m_fd{-1},
    m_dev{nullptr},
    m_errbuf{}
{
    int fd{-1};
    ::libevdev* dev{nullptr};

    fd = ::open(devnode.data(), O_RDONLY|O_NONBLOCK);
    if (fd == -1)
    {
        m_errbuf = "Error: " + std::string{devnode} + "does not exist";
        return;
    }

    int err = ::libevdev_new_from_fd(fd, &dev);
    if (err != 0)
    {
        m_errbuf = "libevdev_new_from_fd Error: " + std::string{std::strerror(-err)};
        ::close(fd);
        return;
    }

    err = ::libevdev_grab(dev, LIBEVDEV_GRAB);
    if (err != 0)
    {
        m_errbuf = "libevdev_grab Error: " + std::string{std::strerror(-err)};
        ::libevdev_free(dev);
        ::close(fd);
        return;
    }

    m_fd = fd;
    m_dev = dev;
}
DeviceGrabber::DeviceGrabber(DeviceGrabber&& other)
    : m_fd{other.m_fd},
    m_dev{other.m_dev},
    m_errbuf{other.m_errbuf}
{
    other.m_fd = -1;
    other.m_dev = nullptr;
}
DeviceGrabber::~DeviceGrabber()
{
    this->close();
}
void DeviceGrabber::close()
{
    if (m_dev != nullptr)
    {
        ::libevdev_grab(m_dev, LIBEVDEV_UNGRAB);
        ::libevdev_free(m_dev);
    }
    if (m_fd != -1)
    {
        ::close(m_fd);
    }
}
