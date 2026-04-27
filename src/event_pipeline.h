#ifndef EVENT_PIPELINE_H
#define EVENT_PIPELINE_H

#include "device_grabber.h"
#include "lua_runtime.h"
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <atomic>
#include <variant>

typedef void (*event_callback_t)(uint32_t device_id, const input_event* ev, void* usr_data);

struct DeviceWatch {
    DeviceGrabber* grabber;
    uint32_t device_id;
    explicit DeviceWatch(DeviceGrabber* grabber_, uint32_t device_id_)
        : grabber{grabber_}, device_id{device_id_}
    {}
};
struct TimerWatch {
    ActiveJob* job;
    uint32_t device_id;
    explicit TimerWatch(ActiveJob* job_, uint32_t device_id_)
        : job{job_}, device_id{device_id_}
    {}
};
using Watch = std::variant<DeviceWatch, TimerWatch>;

class EventPipeline {
public:
    explicit EventPipeline(LuaRuntime& lua, event_callback_t evcb = nullptr, void* usr_data = nullptr);

    EventPipeline(EventPipeline&& other) = delete;
    EventPipeline& operator=(EventPipeline&&) = delete;
    EventPipeline(const EventPipeline&) = delete;
    EventPipeline& operator=(const EventPipeline&) = delete;

    ~EventPipeline();

    bool add_device(DeviceGrabber& g, uint32_t ref_id);

    bool run(std::atomic<bool>& stop);

    explicit operator bool() const { return m_errbuf.empty() && m_lua.errmsg().empty(); }
    std::string errmsg() const { return m_errbuf + std::string{m_lua.errmsg()}; }

private:
    bool register_timer(uint32_t device_id, ActiveJob& aj);
    bool start_job(uint32_t device_id, ActiveJob& aj);
    bool finish_job(uint32_t device_id, ActiveJob& aj);
    bool drain(Watch& w);
    bool step_active_job(uint32_t device_id, ActiveJob& aj);


    LuaRuntime& m_lua;
    event_callback_t m_evcb;
    void* m_usr_data;
    int m_epfd{-1};
    std::vector<std::unique_ptr<Watch>> m_watches;
    std::string m_errbuf;
};

#endif /* #ifndef EVENT_PIPELINE_H */
