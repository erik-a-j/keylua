
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
#include "event_codes_map.h"

#define INIT_LUA "init.lua"
#define USER_CONFIG_REL_TO_HOME ".config/keylua/" INIT_LUA

namespace std { namespace fs = std::filesystem; }

static std::atomic<bool> g_stop{false};
static void signal_handler(int)
{
    g_stop.store(true);
}

/* void print_device_info(const Device& d)
 {
     std::cout << "- Product:Vendor: " << d.pid() << ':' << d.vid() << '\n'
         << "  - Event Nodes:\n";
     for (const auto& node : d.input_interfaces())
     {
         std::cout << "    - Name: " << node.name() << '\n'
             << "      Path: " << node.devpath() << '\n'
             << "      Node: " << node.devnode() << '\n'
             << "      Type: " << node.typestr() << '\n';
     }
 } */

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

    const auto& devices = lua.devices();
    const auto& jobs = lua.jobs();

    std::cout << "devices count: " << devices.size() << std::endl;
    std::cout << "jobs count: " << jobs.size() << std::endl;

    /*   for (size_t i = 0; i < jobs.size(); ++i)
      {
          std::cout << "job#" << i << ":\n";
          for (size_t j = 0; j < jobs[i].atoms.size(); ++j)
          {
              std::cout << "  atom#" << j << ":\n";
              std::cout << "    code: " << std::format("{:#06x}", jobs[i].atoms[j].code) << '\n';
              std::cout << "    value: " << jobs[i].atoms[j].value << std::endl;
          }
      } */

      //return 0;
      /* for (const auto& d : lua.devices())
      {
          std::cout << "Device: \n  name: " << d.name
              << "\n  vid: " << std::hex << d.vid << std::dec
              << "\n  pid: " << std::hex << d.pid << std::dec << std::endl;
          std::cout << "  Mappings: \n";
          for (const auto& m : d.mappings)
          {
              std::cout << "    trigger: " << m.first << '\n'
                  << "    event: " << m.second << std::endl;
          }
      } */

    EventPipeline pipeline{lua};
    if (!pipeline)
    {
        std::cerr << pipeline.errmsg() << std::endl;
        return 1;
    }

    udev* udev = udev_new();
    std::vector<DeviceGrabber> grabbers;
    std::vector<VirtualDevice> vdevices;
    grabbers.reserve(lua.devices().size());
    vdevices.reserve(lua.devices().size());

    for (uint32_t id = 0; id < lua.devices().size(); ++id)
    {
        const auto& cfg = lua.devices()[id];

        Device d{udev, cfg.vid, cfg.pid, cfg.iface};
        if (!d)
        {
            std::cerr << "could not find device: " << std::hex << cfg.vid << ':' << cfg.pid << std::dec << " type=" << d.typestr() << std::endl;
            return 1;
        }

        grabbers.emplace_back(d.devnode());
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

    if (!pipeline.run(g_stop))
    {
        std::cerr << pipeline.errmsg() << std::endl;
    }

    udev_unref(udev);
    return 0;
}
