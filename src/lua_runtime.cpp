
#include "lua_runtime.h"
#include "virtual_device.h"
#include <lua.hpp>
#include <unistd.h>
#include <cstdlib>
#include <string>

#define KEYLUA_RUNTIME "keylua_runtime"
#define USER_CONFIG_REL_TO_HOME ".config/keylua/init.lua"

#define STRIFY_(a) #a
#define STRIFY(a) STRIFY_(a)
#define CAT_(a, b) a ## b
#define CAT(a, b) CAT_(a, b)

#define LuaRuntime_REGISTER(L, F) lua_register(L, STRIFY(F), &dispatch<&LuaRuntime::CAT(l_, F)>)

typedef int (LuaRuntime::* lfunc)(lua_State* L);

template <lfunc F>
static int dispatch(lua_State* L)
{
    LuaRuntime* p = *static_cast<LuaRuntime**>(lua_getextraspace(L));
    return ((*p).*F)(L);
}

LuaRuntime::LuaRuntime()
    : //vdev{vdev_},
    m_lua_state{nullptr}
{
    const char* user_home = std::getenv("HOME");
    if (!user_home)
    {
        m_errbuf = "getenv Error: HOME is not a set env var";
        return;
    }

    std::string user_config = std::string{user_home} + "/" USER_CONFIG_REL_TO_HOME;
    if (0 != ::access(user_config.c_str(), R_OK))
    {
        m_errbuf = "access Error: could not read user config file ~/" USER_CONFIG_REL_TO_HOME;
        return;
    }

    m_lua_state = ::luaL_newstate();
    auto L = m_lua_state;
    luaL_openlibs(L);

    *static_cast<LuaRuntime**>(lua_getextraspace(L)) = this;

    LuaRuntime_REGISTER(L, map);
    //LuaRuntime_REGISTER(L, suppress);

    if (LUA_OK != luaL_dofile(L, user_config.c_str()))
    {
        m_errbuf = "luaL_dofile Error: " + std::string{lua_tostring(L, -1)};
        lua_pop(L, 1);
        return;
    }
}

LuaRuntime::~LuaRuntime()
{
    if (m_lua_state != nullptr)
    {
        ::lua_close(m_lua_state);
    }
}

int LuaRuntime::l_map(::lua_State* L)
{
    int n = lua_gettop(L);
    if (n != 2)
    {
        m_errbuf += "\nmap Error: Expected 2 arguments";
    }
    else if (lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TSTRING)
    {
        m_errbuf += "\nmap Error: Arguments must be strings(for now)";
    }
    else
    {
        m_map.push_back({lua_tostring(L, 1), lua_tostring(L, 2)});
    }
    return 0;
}

/* int LuaRuntime::l_suppress(::lua_State* L)
 {
     printf("l_suppress called\n");
     return 0;
 }
  */