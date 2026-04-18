
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

void print_device_info(const Device& d)
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
}

int main(int argc, char* argv[])
{
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
    LuaRuntime lr{user_config.c_str()};
    if (!lr)
    {
        std::cerr << lr.errmsg() << std::endl;
        return 1;
    }
    std::cout << "Initialized LuaRuntime" << std::endl;

    for (const auto& d : lr.devices())
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
    }

    return 0;

    struct sigaction sa {};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    udev* udev = udev_new();

    const char* vid = "1532";
    const char* pid = "0015";
    Device d{udev, vid, pid};
    print_device_info(d);

    std::cout << std::endl;

    for (const auto& node : d.input_interfaces())
    {
        if (node.type() == InputInterface::Type::Keyboard)
        {
            DeviceGrabber dev{node.devnode()};
            if (!dev)
            {
                std::cerr << dev.errmsg() << std::endl;
                break;
            }
            std::cout << "Grabbed: " << node.devnode() << " Type: " << node.type() << std::endl;

            VirtualDevice vdev{dev.dev()};
            if (!vdev)
            {
                std::cerr << vdev.errmsg() << std::endl;
                break;
            }
            std::cout << "Created VirtualDevice" << std::endl;

            EventPipeline evpl{dev, lr};
            if (!evpl.run(g_stop))
            {
                std::cerr << evpl.errmsg() << std::endl;
                break;
            }

            std::cout << "Ungrabbed: " << node.devnode() << std::endl;
        }
    }

    udev_unref(udev);
    return 0;
}
