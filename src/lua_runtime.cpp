
#include "lua_runtime.h"
#include "virtual_device.h"
#include <lua.hpp>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <libevdev/libevdev.h>

#define KEYLUA_DEVICE_MT "keylua.Device"
#define KEYLUA_EVENTJOB_MT "keylua.EventJob"

typedef int (LuaRuntime::* lfunc)(lua_State* L);
template <lfunc F>
static int dispatch(lua_State* L)
{
    LuaRuntime* p = *static_cast<LuaRuntime**>(lua_getextraspace((L)));
    return ((*p).*F)(L);
}

void LuaRuntime::init_lua()
{
    m_lua_state = ::luaL_newstate();
    lua_State* L = m_lua_state;
    luaL_openlibs(L);

    *static_cast<LuaRuntime**>(lua_getextraspace(L)) = this;


    ::luaL_newmetatable(L, KEYLUA_DEVICE_MT);

    // Build the method table
    lua_newtable(L);
    lua_pushcfunction(L, &dispatch<&LuaRuntime::l_dev_map>);
    ::lua_setfield(L, -2, "map");
    // Stack now: [metatable, methods_table]
    ::lua_setfield(L, -2, "__index");
    // Stack now: [metatable]  (with __index = methods_table set)
    lua_pop(L, 1);

    ::luaL_newmetatable(L, KEYLUA_EVENTJOB_MT);
    lua_pushcfunction(L, &dispatch<&LuaRuntime::l_job_concat>);
    lua_setfield(L, -2, "__concat");
    lua_pop(L, 1);

    lua_register(L, "device", &dispatch<&LuaRuntime::l_device>);
    lua_register(L, "keydown", &dispatch<&LuaRuntime::l_keydown>);
    lua_register(L, "keyup", &dispatch<&LuaRuntime::l_keyup>);
    lua_register(L, "key", &dispatch<&LuaRuntime::l_key>);
}

LuaRuntime::LuaRuntime(const char* config_path)
{
    if (0 != ::access(config_path, R_OK))
    {
        m_errbuf = "access Error: could not read user config: " + std::string{config_path};
        return;
    }

    this->init_lua();

    if (LUA_OK != luaL_dofile(m_lua_state, config_path))
    {
        m_errbuf = "luaL_dofile Error: " + std::string{lua_tostring(m_lua_state, -1)};
        lua_pop(m_lua_state, 1);
    }
}
LuaRuntime::~LuaRuntime()
{
    if (m_lua_state == nullptr) return;

    for (const auto& job : m_jobs)
    {
        std::visit([&](const auto& j) {
            using T = std::decay_t<decltype(j)>;
            if constexpr (std::is_same_v<T, LuaFunctionJob>)
            {
                ::luaL_unref(m_lua_state, LUA_REGISTRYINDEX, j.lua_ref);
            }
        }, job);
    }

    ::lua_close(m_lua_state);
}

bool LuaRuntime::add_virtual_device(VirtualDevice& v, uint32_t ref_id)
{
    if (ref_id >= m_devices.size())
    {
        m_errbuf += "\nbind_device Error: ref_id >= number of devices";
        return false;
    }
    m_devices[ref_id].vdev = &v;
    return true;
}

uint32_t LuaRuntime::new_job(EventJob&& job)
{
    m_jobs.push_back(std::move(job));
    return static_cast<uint32_t>(m_jobs.size() - 1);
}
void LuaRuntime::push_job_ref(::lua_State* L, uint32_t id)
{
    auto* ref = static_cast<EventJobRef*>(::lua_newuserdatauv(L, sizeof(EventJobRef), 0));
    ref->id = id;
    ::luaL_setmetatable(L, KEYLUA_EVENTJOB_MT);
}
uint32_t LuaRuntime::noop_job()
{
    if (!m_noop_job_id.has_value())
    {
        m_noop_job_id = new_job(AtomSequenceJob{{}});
    }
    return m_noop_job_id.value();
}

bool LuaRuntime::process_event(uint32_t device_id, const ::input_event& ev)
{
    DeviceConfig& dev_cfg = m_devices[device_id];
    if (dev_cfg.vdev == nullptr)
    {
        m_errbuf = "process_event Error: virtual device not set";
        return false;
    }

    if (ev.type == EV_KEY)
    {
        auto it = dev_cfg.mappings.find(ev.code);
        if (it != dev_cfg.mappings.end())
        {
            const std::optional<uint32_t>& slot = it->second[static_cast<size_t>(ev.value)];
            if (slot.has_value())
            {
                const EventJob& job = m_jobs[slot.value()];
                return std::visit([&](const auto& j) -> bool
                {
                    using T = std::decay_t<decltype(j)>;

                    if constexpr (std::is_same_v<T, AtomSequenceJob>)
                    {
                        for (const InputAtom& atom : j.atoms)
                        {
                            if (!dev_cfg.vdev->emit(atom.type, atom.code, atom.value)) { return false; }
                        }
                        return dev_cfg.vdev->emit(EV_SYN, SYN_REPORT, 0);
                    }
                    else if constexpr (std::is_same_v<T, LuaFunctionJob>)
                    {
                        ::lua_rawgeti(m_lua_state, LUA_REGISTRYINDEX, j.lua_ref);
                        ::lua_pushinteger(m_lua_state, ev.value);
                        if (lua_pcall(m_lua_state, 1, 0, 0) != LUA_OK)
                        {
                            m_errbuf = lua_tostring(m_lua_state, -1);
                            lua_pop(m_lua_state, 1);
                            return false;
                        }
                        return true;
                    }
                }, job);
            }
        }
    }

    return dev_cfg.vdev->emit(ev.type, ev.code, ev.value);
}

int LuaRuntime::l_device(::lua_State* L)
{
    ::luaL_checktype(L, 1, LUA_TTABLE);

    DeviceConfig cfg{};

    ::lua_getfield(L, 1, "vid");
    if (!::lua_isinteger(L, -1))
    {
        return ::luaL_error(L, "device: 'vid' must be an integer");
    }
    cfg.vid = static_cast<uint16_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    ::lua_getfield(L, 1, "pid");
    if (!::lua_isinteger(L, -1))
    {
        return ::luaL_error(L, "device: 'pid' must be an integer");
    }
    cfg.pid = static_cast<uint16_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    ::lua_getfield(L, 1, "iface");
    if (!::lua_isstring(L, -1))
    {
        return ::luaL_error(L, "device: 'iface' must be a string");
    }
    const char* iface = lua_tostring(L, -1);
    if (0 == std::strcmp(iface, "keyboard"))
    {
        cfg.type |= DEVICETYPE_KEYBOARD;
    }
    else if (0 == std::strcmp(iface, "mouse"))
    {
        cfg.type |= DEVICETYPE_MOUSE;
    }
    else
    {
        return ::luaL_error(L, "device: 'iface' must be either \"keyboard\" or \"mouse\"");
    }

    ::lua_getfield(L, 1, "name");
    cfg.name = ::lua_isstring(L, -1) ? lua_tostring(L, -1) : "keylua virtual device";
    lua_pop(L, 1);

    m_devices.push_back(std::move(cfg));
    uint32_t id = static_cast<uint32_t>(m_devices.size() - 1);

    auto* ref = static_cast<DeviceRef*>(::lua_newuserdatauv(L, sizeof(DeviceRef), 0));
    ref->id = id;
    ::luaL_setmetatable(L, KEYLUA_DEVICE_MT);
    return 1;
}

int LuaRuntime::l_dev_map(lua_State* L)
{
    auto* ref = static_cast<DeviceRef*>(::luaL_checkudata(L, 1, KEYLUA_DEVICE_MT));
    DeviceConfig& dev = m_devices[ref->id];

    const char* trigger = luaL_checkstring(L, 2);

    int evcode = libevdev_event_code_from_name(EV_KEY, trigger);
    if (evcode == -1) { return ::luaL_error(L, "map: unknown key '%s'", trigger); }
    uint16_t trigger_code = static_cast<uint16_t>(evcode);

    std::optional<uint32_t> press_job_id = std::nullopt;
    std::optional<uint32_t> release_job_id = std::nullopt;
    std::optional<uint32_t> repeat_job_id = std::nullopt;

    int arg_type = ::lua_type(L, 3);

    if (arg_type == LUA_TSTRING)
    {
        // shorthand
        const char* name = lua_tostring(L, 3);
        if (name[0] != '\0')
        {
            evcode = libevdev_event_code_from_name(EV_KEY, name);
            if (evcode == -1) { return ::luaL_error(L, "map: unknown key '%s'", name); }

            press_job_id = new_job(AtomSequenceJob{{
                {EV_KEY, static_cast<uint16_t>(evcode), 1}
            }});
            release_job_id = new_job(AtomSequenceJob{{
                { EV_KEY, static_cast<uint16_t>(evcode), 0 }
            }});
            repeat_job_id = new_job(AtomSequenceJob{{
                { EV_KEY, static_cast<uint16_t>(evcode), 2 }
            }});
        }
    }
    else if (arg_type == LUA_TFUNCTION)
    {
        lua_pushvalue(L, 3);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        uint32_t id = new_job(LuaFunctionJob{ref});
        press_job_id = id;
        release_job_id = id;
        repeat_job_id = id;
    }
    else if (arg_type == LUA_TTABLE)
    {

        auto resolve_field = [&](const char* field, int32_t value) -> std::optional<uint32_t>
        {
            ::lua_getfield(L, 3, field);
            int ftype = ::lua_type(L, -1);
            std::optional<uint32_t> result = std::nullopt;

            if (ftype == LUA_TNIL) { ; /* field omitted */ }
            else if (ftype == LUA_TSTRING)
            {
                const char* name = lua_tostring(L, -1);
                if (name[0] == '\0')
                {
                    ::lua_pop(L, 1);
                    return std::nullopt;
                }
                int evcode = libevdev_event_code_from_name(EV_KEY, name);
                if (evcode == -1)
                {
                    ::lua_pop(L, 1);
                    ::luaL_error(L, "map: unknown key '%s' in %s", name, field);
                    return std::nullopt;
                }
                if (value == 2)
                {
                    result = new_job(AtomSequenceJob{{
                        {EV_KEY, static_cast<uint16_t>(evcode), 1},
                        {EV_KEY, static_cast<uint16_t>(evcode), 0}
                    }});
                }
                else
                {
                    result = new_job(AtomSequenceJob{{
                        {EV_KEY, static_cast<uint16_t>(evcode), value}
                    }});
                }
            }
            else if (ftype == LUA_TFUNCTION)
            {
                lua_pushvalue(L, -1);
                int ref = luaL_ref(L, LUA_REGISTRYINDEX);
                result = new_job(LuaFunctionJob{ref});
            }
            else if (ftype == LUA_TUSERDATA)
            {
                auto* jref = static_cast<EventJobRef*>(::luaL_checkudata(L, -1, KEYLUA_EVENTJOB_MT));
                result = jref->id;
            }
            else
            {
                ::lua_pop(L, 1);
                ::luaL_error(L, "map: '%s' must be [keylua.KeyName|keylua.EventJob|function]", field);
                return std::nullopt;
            }

            ::lua_pop(L, 1);
            return result;
        };

        press_job_id = resolve_field("on_press", 1);
        release_job_id = resolve_field("on_release", 0);
        repeat_job_id = resolve_field("on_repeat", 2);

        if (!press_job_id.has_value() && !release_job_id.has_value() && !repeat_job_id.has_value())
        {
            return ::luaL_error(L, "map: table must have at least one of 'on_press', 'on_release' or 'on_repeat'");
        }
    }
    else if (arg_type == LUA_TUSERDATA)
    {
        return ::luaL_error(L, "map: pass an EventJob via keylua.TriggerConfig");
    }
    else
    {
        return ::luaL_error(L, "map: second argument must be [keylua.KeyName|keylua.TriggerConfig|function]");
    }

    if (!release_job_id.has_value()) { release_job_id = noop_job(); }
    if (!press_job_id.has_value()) { press_job_id = noop_job(); }
    if (!repeat_job_id.has_value()) { repeat_job_id = noop_job(); }
    dev.mappings[trigger_code][0] = release_job_id;
    dev.mappings[trigger_code][1] = press_job_id;
    dev.mappings[trigger_code][2] = repeat_job_id;
    return 0;
}

#define KEYACTION_PRESS (1 << 0)
#define KEYACTION_RELEASE (1 << 1)

int LuaRuntime::l_keydown(::lua_State* L) { return this->impl_key(L, "keydown", KEYACTION_PRESS); }
int LuaRuntime::l_keyup(::lua_State* L) { return this->impl_key(L, "keyup", KEYACTION_RELEASE); }
int LuaRuntime::l_key(::lua_State* L) { return this->impl_key(L, "key", KEYACTION_PRESS | KEYACTION_RELEASE); }
int LuaRuntime::impl_key(::lua_State* L, const char* fname, int32_t key_action)
{
    const char* name = luaL_checkstring(L, 1);

    int evcode = libevdev_event_code_from_name(EV_KEY, name);
    if (evcode == -1) { return luaL_error(L, "%s: unknown key '%s'", fname, name); }

    std::vector<InputAtom> atoms;
    if (key_action & KEYACTION_PRESS) { atoms.push_back({EV_KEY, static_cast<uint16_t>(evcode), 1}); }
    if (key_action & KEYACTION_RELEASE) { atoms.push_back({EV_KEY, static_cast<uint16_t>(evcode), 0}); }

    uint32_t id = new_job(AtomSequenceJob{std::move(atoms)});
    push_job_ref(L, id);
    return 1;
}

int LuaRuntime::l_job_concat(lua_State* L)
{
    auto* a = static_cast<EventJobRef*>(luaL_checkudata(L, 1, KEYLUA_EVENTJOB_MT));
    auto* b = static_cast<EventJobRef*>(luaL_checkudata(L, 2, KEYLUA_EVENTJOB_MT));

    const EventJob& ja = m_jobs[a->id];
    const EventJob& jb = m_jobs[b->id];

    if (!std::holds_alternative<AtomSequenceJob>(ja) || !std::holds_alternative<AtomSequenceJob>(jb))
    {
        return luaL_error(L, "concat: cannot concatenate function jobs");
    }

    const auto& sa = std::get<AtomSequenceJob>(ja);
    const auto& sb = std::get<AtomSequenceJob>(jb);

    AtomSequenceJob merged;
    merged.atoms.reserve(sa.atoms.size() + sb.atoms.size());
    merged.atoms.insert(merged.atoms.end(), sa.atoms.begin(), sa.atoms.end());
    merged.atoms.insert(merged.atoms.end(), sb.atoms.begin(), sb.atoms.end());

    uint32_t id = new_job(std::move(merged));
    push_job_ref(L, id);
    return 1;
}