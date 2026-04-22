#ifndef EVENT_PIPELINE_H
#define EVENT_PIPELINE_H

#include "device_grabber.h"
#include "lua_runtime.h"
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <atomic>

typedef void (*event_callback_t)(uint32_t device_id, const input_event* ev, void* usr_data);

struct Watch {
    DeviceGrabber* grabber;
    uint32_t device_id;
};

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

    bool drain(Watch& w);

    LuaRuntime& m_lua;
    event_callback_t m_evcb;
    void* m_usr_data;
    int m_epfd{-1};
    std::vector<std::unique_ptr<Watch>> m_watches;
    std::string m_errbuf;
};

#endif /* #ifndef EVENT_PIPELINE_H */
