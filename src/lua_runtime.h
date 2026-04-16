#ifndef LUA_RUNTIME_H
#define LUA_RUNTIME_H

#include <string>
#include <string_view>
#include <memory>

class VirtualDevice;
struct lua_State;

class LuaRuntime {
public:
    bool suppress{false};
    VirtualDevice& vdev;

    LuaRuntime(VirtualDevice& vdev_);
    ~LuaRuntime();

    explicit operator bool() const { return m_errbuf.empty() && m_lua_state != nullptr; }
    std::string_view errmsg() const { return m_errbuf; }

private:

    int l_suppress(::lua_State* L);

    ::lua_State* m_lua_state;
    std::string m_errbuf;
};

#endif /* #ifndef LUA_RUNTIME_H */
