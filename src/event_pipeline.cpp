
#include "event_pipeline.h"
#include "device_grabber.h"
#include "virtual_device.h"
#include "lua_runtime.h"
#include <libevdev/libevdev.h>
#include <sys/epoll.h>
#include <cerrno>
#include <cstring>

EventPipeline::EventPipeline(LuaRuntime& lua)
    : m_lua{lua}
{
    m_epfd = ::epoll_create1(EPOLL_CLOEXEC);
    if (m_epfd == -1)
    {
        m_errbuf = "epoll_create1 Error: " + std::string{std::strerror(errno)};
    }
}


EventPipeline::~EventPipeline()
{
    if (m_epfd != -1) { ::close(m_epfd); }
}

bool EventPipeline::add_device(DeviceGrabber& g, uint32_t ref_id)
{
    auto w = std::make_unique<Watch>(Watch{&g, ref_id});

    ::epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.ptr = w.get();

    if (::epoll_ctl(m_epfd, EPOLL_CTL_ADD, g.fd(), &ev) == -1)
    {
        m_errbuf = "epoll_ctl: " + std::string{std::strerror(errno)};
        return false;
    }

    m_watches.push_back(std::move(w));
    return true;
}

bool EventPipeline::run(std::atomic<bool>& stop)
{
    constexpr int MAX_EVENTS = 16;
    ::epoll_event events[MAX_EVENTS];

    while (!stop.load())
    {
        int n = ::epoll_wait(m_epfd, events, MAX_EVENTS, 100);
        if (n == -1)
        {
            if (errno == EINTR) { continue; }
            m_errbuf = "epoll_wait: " + std::string{std::strerror(errno)};
            return false;
        }

        for (int i = 0; i < n; ++i)
        {
            auto* w = static_cast<Watch*>(events[i].data.ptr);

            if (events[i].events & (EPOLLERR | EPOLLHUP))
            {
                // Device unplugged. Remove it and continue with others.
                ::epoll_ctl(m_epfd, EPOLL_CTL_DEL, w->grabber->fd(), nullptr);
                // Optionally: erase from m_watches, or mark dead.
                continue;
            }

            if (events[i].events & EPOLLIN)
            {
                if (!drain(*w)) return false;
            }
        }
    }
    return true;
}

bool EventPipeline::drain(Watch& w)
{
    input_event ie{};
    int rc;
    while ((rc = ::libevdev_next_event(w.grabber->dev(),
        LIBEVDEV_READ_FLAG_NORMAL, &ie)) >= 0)
    {
        if (rc == LIBEVDEV_READ_STATUS_SYNC)
        {
            while ((rc = ::libevdev_next_event(w.grabber->dev(),
                LIBEVDEV_READ_FLAG_SYNC, &ie)) >= 0)
            {
                if (!m_lua.process_event(w.device_id, ie)) return false;
            }
        }
        else
        {
            if (!m_lua.process_event(w.device_id, ie)) return false;
        }
    }
    return true;
}

/* bool EventPipeline::run(std::atomic<bool>& stop)
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
                         if (!m_lua.process_event(ie))
                         {
                             status = false;
                             break;
                         }
                     }
                     if (!status) break;
                 }
                 else // LIBEVDEV_READ_STATUS_SUCCESS
                 {
                     if (!m_lua.process_event(ie))
                     {
                         status = false;
                         break;
                     }
                 }
             }
         }
     }
     ::close(epfd);
     return status;
 } */
