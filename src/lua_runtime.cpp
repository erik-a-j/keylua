
#include "lua_runtime.h"
#include "virtual_device.h"
#include "input_event_codes_map.h"
#include <lua.hpp>
#include <unistd.h>
#include <cstdlib>
#include <string>

#define KEYLUA_EVENTJOB_MT "keylua.EventJob"

typedef int (LuaRuntime::* lfunc)(lua_State* L);
template <lfunc F>
static int dispatch(lua_State* L)
{
    LuaRuntime* p = *static_cast<LuaRuntime**>(lua_getextraspace((L)));
    return ((*p).*F)(L);
}
#define REGISTER(L, NAME, METHOD) lua_register(L, NAME, &dispatch<&LuaRuntime::METHOD>)

LuaRuntime::LuaRuntime(VirtualDevice& vdev, const char* config_path)
    : m_vdev{vdev}
{
    if (0 != ::access(config_path, R_OK))
    {
        m_errbuf = "access Error: could not read user config: " + std::string{config_path};
        return;
    }

    m_lua_state = ::luaL_newstate();
    auto L = m_lua_state;
    luaL_openlibs(L);

    *static_cast<LuaRuntime**>(lua_getextraspace(L)) = this;

    ::luaL_newmetatable(L, KEYLUA_EVENTJOB_MT);
    lua_pop(L, 1);

    REGISTER(L, "map", l_map);
    REGISTER(L, "keydown", l_keydown);
    REGISTER(L, "keyup", l_keyup);
    REGISTER(L, "key", l_key);

    if (LUA_OK != luaL_dofile(L, config_path))
    {
        m_errbuf = "luaL_dofile Error: " + std::string{lua_tostring(L, -1)};
        lua_pop(L, 1);
    }
}
LuaRuntime::~LuaRuntime()
{
    if (m_lua_state != nullptr)
    {
        ::lua_close(m_lua_state);
    }
}

uint32_t LuaRuntime::new_job(std::vector<InputAtom> atoms)
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

bool LuaRuntime::process_event(const::input_event& ev)
{
    return m_vdev.emit(ev.type, ev.code, ev.value);
}

int LuaRuntime::l_map(::lua_State* L)
{
    if (::lua_gettop(L) != 2)
    {
        return ::luaL_error(L, "map: expected 2 arguments, got %d", ::lua_gettop(L));
    }

    const char* trigger = luaL_checkstring(L, 1);
    const keyword* kw = symbol_to_evcode(trigger);
    if (!kw)
    {
        return ::luaL_error(L, "map: unknown key '%s'", trigger);
    }

    uint16_t trigger_code = static_cast<uint16_t>(kw->code);
    uint32_t job_id;

    if (::lua_type(L, 2) == LUA_TSTRING)
    {
        // Shorthand: "KEY_A" becomes a press+release job.
        const char* name = lua_tostring(L, 2);
        kw = symbol_to_evcode(name);
        if (!kw)
        {
            return ::luaL_error(L, "map: unknown key '%s'", name);
        }
        job_id = new_job({
            { EV_KEY, static_cast<uint16_t>(kw->code), 1 },
            { EV_KEY, static_cast<uint16_t>(kw->code), 0 },
        });
    }
    else
    {
        // Full form: an EventJob userdata.
        auto* ref = static_cast<EventJobRef*>(::luaL_checkudata(L, 2, KEYLUA_EVENTJOB_MT));
        job_id = ref->id;
    }

    m_map[trigger_code] = job_id;
    return 0;
}

#define KEYACTION_PRESS (1 << 0)
#define KEYACTION_RELEASE (1 << 1)

int LuaRuntime::l_keydown(::lua_State* L) { return this->impl_key(L, "keydown", KEYACTION_PRESS); }
int LuaRuntime::l_keyup(::lua_State* L) { return this->impl_key(L, "keyup", KEYACTION_RELEASE); }
int LuaRuntime::l_key(::lua_State* L) { return this->impl_key(L, "key", KEYACTION_PRESS & KEYACTION_RELEASE); }
int LuaRuntime::impl_key(::lua_State* L, const char* fname, int32_t key_action)
{
    const char* name = luaL_checkstring(L, 1);

    const keyword* kw = symbol_to_evcode(name);
    if (!kw)
    {
        return luaL_error(L, "%s: unknown key '%s'", fname, name);
    }

    std::vector<InputAtom> atoms;
    if (key_action & KEYACTION_PRESS)
    {
        atoms.push_back({EV_KEY, static_cast<uint16_t>(kw->code), 1});
    }
    if (key_action & KEYACTION_RELEASE)
    {
        atoms.push_back({EV_KEY, static_cast<uint16_t>(kw->code), 0});
    }

    uint32_t id = new_job(std::move(atoms));
    push_job_ref(L, id);
    return 1;
}
