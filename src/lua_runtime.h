#ifndef LUA_RUNTIME_H
#define LUA_RUNTIME_H

#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <memory>

class VirtualDevice;
struct lua_State;

class LuaRuntime {
public:
    //bool suppress{false};
    //VirtualDevice& vdev;

    LuaRuntime();
    ~LuaRuntime();

    const std::vector<std::array<std::string, 2>>& mapping() const { return m_map; }

    explicit operator bool() const { return m_errbuf.empty() && m_lua_state != nullptr; }
    std::string_view errmsg() const { return m_errbuf; }

private:

    int l_map(::lua_State* L);
    //int l_suppress(::lua_State* L);

    ::lua_State* m_lua_state;
    std::vector<std::array<std::string, 2>> m_map;
    std::string m_errbuf;
};

#endif /* #ifndef LUA_RUNTIME_H */
