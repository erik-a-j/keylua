
#include <fcntl.h>
#include <cstdlib>
#include <iostream>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <string>
#include <filesystem>
#include <signal.h>
#include <unistd.h>
#include "device.h"
#include "device_grabber.h"
#include "virtual_device.h"
#include "event_pipeline.h"
#include "lua_runtime.h"

#define INIT_LUA "init.lua"
#define USER_CONFIG_REL_TO_HOME ".config/keylua/" INIT_LUA

namespace std { namespace fs = std::filesystem; }

static std::atomic<bool> g_stop{false};
static void signal_handler(int)
{
    g_stop.store(true);
}

struct usr_data_t {
    const std::vector<DeviceConfig>& devices;
    const std::vector<EventJob>& jobs;
};

void event_callback(uint32_t device_id, const input_event* ev, void* usr_data)
{
    if (ev->type != EV_KEY) return;

    usr_data_t* x = reinterpret_cast<usr_data_t*>(usr_data);
    const auto& d = x->devices[device_id];

    auto value_to_action = [](int32_t v) -> const char*
    {
        switch (v)
        {
        case 0: return "RELEASE";
        case 1: return "PRESS";
        case 2: return "REPEAT";
        default: return nullptr;
        }
    };

    const char* action = value_to_action(ev->value);
    const char* key = libevdev_event_code_get_name(ev->type, ev->code);

    std::string map = "passthrough";
    {
        auto it = d.mappings.find(ev->code);
        if (it != d.mappings.end())
        {
            const std::optional<uint32_t>& slot = it->second[static_cast<size_t>(ev->value)];
            if (slot.has_value())
            {
                const EventJob& job = x->jobs[slot.value()];
                std::visit([&](const auto& j)
                {
                    using T = std::decay_t<decltype(j)>;

                    if constexpr (std::is_same_v<T, AtomSequenceJob>)
                    {
                        map = "AtomSequenceJob{";
                        for (const auto& atom : j.atoms)
                        {
                            map += "\n  ";
                            const char* seqaction = value_to_action(atom.value);
                            const char* seqkey = libevdev_event_code_get_name(atom.type, atom.code);

                            if (seqaction)
                            {
                                map += seqaction;
                            }
                            else
                            {
                                map += "value(";
                                map += std::to_string(atom.value) + ")";
                            }
                            map += ": ";
                            if (seqkey)
                            {
                                map += seqkey;
                            }
                            else
                            {
                                map += "value(";
                                map += std::to_string(atom.code) + ")";
                            }
                            map += ",";
                        }
                        map += "\n}";
                    }
                    else if constexpr (std::is_same_v<T, LuaFunctionJob>)
                    {
                        map = "LuaFunctionJob";
                    }
                }, job);
            }
        }

        std::cout << d.name << ": action=";
        if (action) std::cout << action;
        else std::cout << "value(" << ev->value << ")";

        std::cout << ", key=";
        if (key) std::cout << key;
        else std::cout << "value(" << ev->code << ")";

        std::cout << ", map=" << map << std::endl;
    }
}

int main(int argc, char* argv[])
{
    struct sigaction sa {};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    std::fs::path user_config;
    if (argc == 2)
    {
        user_config = std::fs::path{argv[1]} / INIT_LUA;
    }
    else
    {
        const char* user_home = std::getenv("HOME");
        if (user_home)
        {
            user_config = std::fs::path{user_home} / USER_CONFIG_REL_TO_HOME;
        }
    }
    if (!std::fs::exists(user_config))
    {
        std::cerr << "config: " << user_config << " does not exist" << std::endl;
        return 1;
    }

    LuaRuntime lua{user_config.c_str()};
    if (!lua)
    {
        std::cerr << lua.errmsg() << std::endl;
        return 1;
    }
    std::cout << "Initialized LuaRuntime" << std::endl;

    usr_data_t usr_data{lua.devices(), lua.jobs()};

    std::cout << "devices count: " << usr_data.devices.size() << std::endl;
    std::cout << "jobs count: " << usr_data.jobs.size() << std::endl;

    EventPipeline pipeline{lua, event_callback, &usr_data};
    if (!pipeline)
    {
        std::cerr << pipeline.errmsg() << std::endl;
        return 1;
    }

    std::vector<DeviceGrabber> grabbers;
    std::vector<VirtualDevice> vdevices;
    grabbers.reserve(lua.devices().size());
    vdevices.reserve(lua.devices().size());

    for (uint32_t id = 0; id < lua.devices().size(); ++id)
    {
        const auto& cfg = lua.devices()[id];

        Device d{cfg.vid, cfg.pid};
        if (!d)
        {
            std::cerr << d.errmsg() << std::endl;
            //std::cerr << "could not find device: " << std::hex << cfg.vid << ':' << cfg.pid << std::dec << std::endl;
            return 1;
        }
        std::cout << std::hex << cfg.vid << ':' << cfg.pid << std::dec << " keyboard: " << d.keyboard_devname() << std::endl;
        std::cout << std::hex << cfg.vid << ':' << cfg.pid << std::dec << " mouse: " << d.mouse_devname() << std::endl;
        continue;

        grabbers.emplace_back("d.devnode()");
        auto& g = grabbers.back();
        if (!g)
        {
            std::cerr << g.errmsg() << std::endl;
            return 1;
        }

        vdevices.emplace_back(g.dev());
        auto& v = vdevices.back();
        if (!v)
        {
            std::cerr << v.errmsg() << std::endl;
            return 1;
        }

        if (!lua.add_virtual_device(v, id))
        {
            std::cerr << lua.errmsg() << std::endl;
            return 1;
        }

        if (!pipeline.add_device(g, id))
        {
            std::cerr << pipeline.errmsg() << std::endl;
            return 1;
        }
    }

    //if (!pipeline.run(g_stop)) { std::cerr << pipeline.errmsg() << std::endl; }

    return 0;
}
