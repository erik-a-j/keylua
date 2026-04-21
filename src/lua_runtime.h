#ifndef LUA_RUNTIME_H
#define LUA_RUNTIME_H

#include "event_job.h"
#include "device_config.h"
#include "virtual_device.h"
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>
#include <utility>

struct lua_State;
struct input_event;

class LuaRuntime {
public:
    LuaRuntime(const char* config_path);
    ~LuaRuntime();

    bool add_virtual_device(VirtualDevice& v, uint32_t ref_id);
    bool process_event(uint32_t device_id, const ::input_event& ev);

    const std::vector<EventJob>& jobs() const { return m_jobs; }
    const std::vector<DeviceConfig>& devices() const { return m_devices; }

    explicit operator bool() const { return m_errbuf.empty() && m_lua_state != nullptr; }
    std::string_view errmsg() const { return m_errbuf; }

private:
    int l_device(::lua_State* L);
    int l_dev_map(lua_State* L);
    int l_keydown(::lua_State* L);
    int l_keyup(::lua_State* L);
    int l_key(::lua_State* L);
    int impl_key(::lua_State* L, const char* fname, int32_t key_action);

    bool do_emit_key(const char* key, int value);
    int emit_key(::lua_State* L, const char* fname, int value);

    uint32_t new_job(EventJob&& atoms);
    void push_job_ref(::lua_State* L, uint32_t id);

    void init_lua();

    ::lua_State* m_lua_state{nullptr};
    std::vector<EventJob> m_jobs;
    std::vector<DeviceConfig> m_devices;
    std::string m_errbuf;
};

#endif /* #ifndef LUA_RUNTIME_H */
