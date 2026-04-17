
#include "event_pipeline.h"
#include "device_grabber.h"
#include "virtual_device.h"
#include <libevdev/libevdev.h>
#include <sys/epoll.h>
#include <cerrno>
#include <cstring>

EventPipeline::EventPipeline(DeviceGrabber& dev, VirtualDevice& vdev, LuaRuntime& lua)
    : m_dev{dev}, m_vdev{vdev}, m_lua{lua}
{}


bool EventPipeline::run(std::atomic<bool>& stop)
{
    int epfd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1)
    {
        m_errbuf = "epoll_create1 Error: " + std::string{std::strerror(errno)};
        return false;
    }

    ::epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = m_dev.fd();
    ::epoll_ctl(epfd, EPOLL_CTL_ADD, m_dev.fd(), &ev);

    bool status{true};
    ::epoll_event events[1];
    while (!stop.load())
    {
        int n = ::epoll_wait(epfd, events, 1, 100);
        if (n == -1)
        {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) continue;

        if (events[0].events & (EPOLLERR | EPOLLHUP))
        {
            break;
        }

        if (events[0].events & EPOLLIN)
        {
            input_event ie{};
            int rc;
            while ((rc = libevdev_next_event(m_dev.dev(), LIBEVDEV_READ_FLAG_NORMAL, &ie)) >= 0)
            {
                if (rc == LIBEVDEV_READ_STATUS_SYNC)
                {
                    while ((rc = libevdev_next_event(m_dev.dev(), LIBEVDEV_READ_FLAG_SYNC, &ie)) >= 0)
                    {
                        if (!this->process_event(ie))
                        {
                            m_errbuf = m_vdev.errmsg();
                            status = false;
                            break;
                        }
                    }
                    if (!status) break;
                }
                else // LIBEVDEV_READ_STATUS_SUCCESS
                {
                    if (!this->process_event(ie))
                    {
                        m_errbuf = m_vdev.errmsg();
                        status = false;
                        break;
                    }
                }
            }
        }
    }
    ::close(epfd);
    return status;
}

bool EventPipeline::process_event(const::input_event& ev)
{
    return m_vdev.emit(ev);
}


