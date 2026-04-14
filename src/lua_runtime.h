#ifndef LUA_RUNTIME_H
#define LUA_RUNTIME_H

#include <string>
#include <string_view>

struct lua_State;

class LuaRuntime {
public:
    LuaRuntime();
    ~LuaRuntime();

    explicit operator bool() const { return m_errbuf.empty() && m_lua_state != nullptr; }
    std::string_view errmsg() const { return m_errbuf; }

private:
    ::lua_State* m_lua_state;
    std::string m_errbuf;
};

#endif /* #ifndef LUA_RUNTIME_H */
