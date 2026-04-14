
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <string>
#include <signal.h>
#include <unistd.h>
#include "device.h"
#include "device_grabber.h"
#include "virtual_device.h"
#include "event_pipeline.h"
#include "lua_runtime.h"

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

int main()
{
    LuaRuntime lr;
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
        if (node.type() == InputInterface::Type::Mouse)
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

            EventPipeline evpl{dev, vdev};
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
