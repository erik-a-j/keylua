
#include "event_pipeline.h"
#include "device_grabber.h"
#include "virtual_device.h"
#include "lua_runtime.h"
#include "utils.h"
#include <cerrno>
#include <cstring>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <libevdev/libevdev.h>

EventPipeline::EventPipeline(LuaRuntime& lua, event_callback_t evcb, void* usr_data)
    : m_lua{lua}, m_evcb{evcb}, m_usr_data{usr_data}
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

bool EventPipeline::register_timer(uint32_t device_id, ActiveJob& aj)
{
    auto tw = std::make_unique<Watch>(TimerWatch{&aj, device_id});

    ::epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.ptr = tw.get();

    if (-1 == ::epoll_ctl(m_epfd, EPOLL_CTL_ADD, aj.timerfd, &ev))
    {
        m_errbuf = "epoll_ctl register_timer: " + std::string{std::strerror(errno)};
        return false;
    }

    m_watches.push_back(std::move(tw));
    return true;
}

bool EventPipeline::start_job(uint32_t device_id, ActiveJob& aj)
{
    if (!step_active_job(device_id, aj)) { return false; }
    return aj.timerfd != -1
        ? register_timer(device_id, aj) : finish_job(device_id, aj);
}

bool EventPipeline::finish_job(uint32_t device_id, ActiveJob& aj)
{
    DeviceConfig& dev = m_lua.devices()[device_id];

    if (!dev.vdev->emit(EV_SYN, SYN_REPORT, 0))
    {
        m_errbuf = "finish_job: SYN_REPORT failed";
        return false;
    }

    auto& jobs = dev.active_jobs;
    for (size_t i = 0; i < jobs.size(); ++i)
    {
        if (jobs[i].get() == &aj)
        {
            if (i != jobs.size() - 1) { jobs[i] = std::move(jobs.back()); }
            jobs.pop_back();
            return true;
        }
    }

    m_errbuf = "finish_job: job not found";
    return false;
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
                if (m_evcb) m_evcb(w.device_id, &ie, m_usr_data);
                if (!m_lua.process_event(w.device_id, ie)) return false;
            }
        }
        else
        {
            if (m_evcb) m_evcb(w.device_id, &ie, m_usr_data);
            if (!m_lua.process_event(w.device_id, ie)) return false;
        }
    }
    return true;
}

bool EventPipeline::step_active_job(uint32_t device_id, ActiveJob& aj)
{
    DeviceConfig& dev_cfg = m_lua.devices()[device_id];
    const auto& atoms = aj.job.atoms;

    while (aj.atom_index < atoms.size())
    {
        const Atom& atom = atoms[aj.atom_index];

        bool sleeping = false;

        bool ok = std::visit(overloaded{
            [&](const InputAtom& a) -> bool
            {
                return dev_cfg.vdev->emit(a.type, a.code, a.value);
            },
            [&](const SleepAtom& a) -> bool
            {
                int tfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
                if (tfd == -1)
                {
                    m_errbuf = "timerfd_create: " + std::string{std::strerror(errno)};
                    return false;
                }

                ::itimerspec ts{};
                ts.it_value.tv_sec = a.ms / 1000;
                ts.it_value.tv_nsec = (a.ms % 1000) * 1'000'000;
                if (-1 == ::timerfd_settime(tfd, 0, &ts, nullptr))
                {
                    m_errbuf = "timerfd_settime: " + std::string{std::strerror(errno)};
                    ::close(tfd);
                    return false;
                }
                aj.timerfd = tfd;
                sleeping = true;
                return true;
            }
        }, atom);

        if (!ok) { return false; }
        else if (sleeping) { return true; }
        ++aj.atom_index;
    }

    return true;
}