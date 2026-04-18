#ifndef EVENT_PIPELINE_H
#define EVENT_PIPELINE_H

#include "device_grabber.h"
#include "lua_runtime.h"
#include <string>
#include <string_view>
#include <atomic>

struct input_event;

class EventPipeline {
public:
    explicit EventPipeline(DeviceGrabber& dev, LuaRuntime& lua);

    EventPipeline(EventPipeline&& other) = delete;
    EventPipeline& operator=(EventPipeline&&) = delete;
    EventPipeline(const EventPipeline&) = delete;
    EventPipeline& operator=(const EventPipeline&) = delete;

    bool run(std::atomic<bool>& stop);
    //void request_stop();

    explicit operator bool() const { return m_errbuf.empty() && m_dev.errmsg().empty() && m_lua.errmsg().empty() && m_lua.vdev()->errmsg().empty(); }
    std::string errmsg() const { return m_errbuf + std::string{m_dev.errmsg()} + std::string{m_lua.errmsg()} + std::string{m_lua.vdev()->errmsg()}; }
private:

    DeviceGrabber& m_dev;
    LuaRuntime& m_lua;
    std::string m_errbuf;
};

#endif /* #ifndef EVENT_PIPELINE_H */
