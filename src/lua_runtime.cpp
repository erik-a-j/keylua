
#include "lua_runtime.h"
#include "virtual_device.h"
#include "event_codes_map.h"
#include <lua.hpp>
#include <unistd.h>
#include <cstdlib>
#include <string>

#define KEYLUA_DEVICE_MT "keylua.Device"
#define KEYLUA_MAPPING_MT "keylua.Mapping"
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
    if (m_lua_state != nullptr)
    {
        ::lua_close(m_lua_state);
    }
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

uint32_t LuaRuntime::new_job(std::vector<InputAtom>&& atoms)
{
    m_jobs.push_back({std::move(atoms)});
    return static_cast<uint32_t>(m_jobs.size() - 1);
}

void LuaRuntime::push_job_ref(::lua_State* L, uint32_t id)
{
    auto* ref = static_cast<EventJobRef*>(::lua_newuserdatauv(L, sizeof(EventJobRef), 0));
    ref->id = id;
    ::luaL_setmetatable(L, KEYLUA_EVENTJOB_MT);
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
            const std::optional<uint32_t>& slot = it->second[ev.value];
            if (slot.has_value())
            {
                const EventJob& job = m_jobs[slot.value()];
                for (const InputAtom& atom : job.atoms)
                {
                    if (!dev_cfg.vdev->emit(atom.type, atom.code, atom.value)) { return false; }
                }
                return dev_cfg.vdev->emit(EV_SYN, SYN_REPORT, 0);
            }
            else
            {
                ; // slot not set for this value (e.g. no on_repeat) — fall through to passthrough
            }
        }
    }

    return dev_cfg.vdev->emit(ev.type, ev.code, ev.value);
}

int LuaRuntime::l_device(::lua_State* L)
{
    ::luaL_checktype(L, 1, LUA_TTABLE);

    DeviceConfig cfg;

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
        cfg.iface = DeviceType::Keyboard;
    }
    else if (0 == std::strcmp(iface, "mouse"))
    {
        cfg.iface = DeviceType::Mouse;
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
    const keyword* kw = EventCodesMap::lookup(trigger);
    if (!kw) { return ::luaL_error(L, "map: unknown key '%s'", trigger); }
    uint16_t trigger_code = static_cast<uint16_t>(kw->code);

    std::optional<uint32_t> press_job_id = std::nullopt;
    std::optional<uint32_t> release_job_id = std::nullopt;
    int arg_type = ::lua_type(L, 3);

    if (arg_type == LUA_TSTRING)
    {
        // shorthand
        const char* name = lua_tostring(L, 3);
        kw = EventCodesMap::lookup(name);
        if (!kw) { return ::luaL_error(L, "map: unknown key '%s'", name); }
        press_job_id = new_job({{ EV_KEY, static_cast<uint16_t>(kw->code), 1 }});
        release_job_id = new_job({{ EV_KEY, static_cast<uint16_t>(kw->code), 0 }});
    }
    else if (arg_type == LUA_TUSERDATA)
    {
        auto* jref = static_cast<EventJobRef*>(::luaL_checkudata(L, 3, KEYLUA_EVENTJOB_MT));
        press_job_id = jref->id;
    }
    else if (arg_type == LUA_TTABLE)
    {
        std::vector<InputAtom> atoms;

        auto resolve_field = [&](const char* field, int32_t value) -> bool
        {
            ::lua_getfield(L, 3, field);
            int ftype = ::lua_type(L, -1);

            if (ftype == LUA_TNIL)
            {
                ; // field omitted, that's fine
            }
            else if (ftype == LUA_TSTRING)
            {
                const char* name = lua_tostring(L, -1);
                const keyword* kw = EventCodesMap::lookup(name);
                if (!kw)
                {
                    ::lua_pop(L, 1);
                    ::luaL_error(L, "map: unknown key '%s' in %s", name, field);
                    return false;
                }
                atoms.push_back({EV_KEY, static_cast<uint16_t>(kw->code), value});
            }
            else if (ftype == LUA_TUSERDATA)
            {
                auto* jref = static_cast<EventJobRef*>(::luaL_checkudata(L, -1, KEYLUA_EVENTJOB_MT));
                const auto& job_atoms = m_jobs[jref->id].atoms;
                atoms.insert(atoms.end(), job_atoms.begin(), job_atoms.end());
            }
            else
            {
                ::lua_pop(L, 1);
                ::luaL_error(L, "map: '%s' must be a string or EventJob", field);
                return false;
            }

            ::lua_pop(L, 1);
            return true;
        };

        if (!resolve_field("on_press", 1)) return 0;
        if (!resolve_field("on_release", 0)) return 0;

        if (atoms.empty())
        {
            return ::luaL_error(L, "map: table must have at least one of 'on_press' or 'on_release'");
        }

        job_id = new_job(std::move(atoms));
    }
    else
    {
        return ::luaL_error(L, "map: second argument must be a string, EventJob, or table");
    }

    dev.mappings[trigger_code] = job_id;
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

    const keyword* kw = EventCodesMap::lookup(name);
    if (!kw) { return luaL_error(L, "%s: unknown key '%s'", fname, name); }

    std::vector<InputAtom> atoms;
    if (key_action & KEYACTION_PRESS) { atoms.push_back({EV_KEY, static_cast<uint16_t>(kw->code), 1}); }
    if (key_action & KEYACTION_RELEASE) { atoms.push_back({EV_KEY, static_cast<uint16_t>(kw->code), 0}); }

    uint32_t id = new_job(std::move(atoms));
    push_job_ref(L, id);
    return 1;
}
