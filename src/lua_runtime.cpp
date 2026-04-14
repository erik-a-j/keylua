
#include "lua_runtime.h"
#include <lua.hpp>
#include <unistd.h>
#include <cstdlib>
#include <string>

#define USER_CONFIG_REL_TO_HOME ".config/keylua/init.lua"

LuaRuntime::LuaRuntime()
    : m_lua_state{nullptr}
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
