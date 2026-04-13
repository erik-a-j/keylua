
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <string>
#include <unistd.h>
#include "device.h"
#include "device_grabber.h"
#include "virtual_device.h"
#include "event_pipeline.h"

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
    udev* udev = udev_new();

    const char* vid = "1532";
    const char* pid = "0015";
    Device d{udev, vid, pid};
    print_device_info(d);

    std::cout << std::endl;

    for (const auto& node : d.input_interfaces())
    {
        DeviceGrabber dev{node.devnode()};
        if (!dev)
        {
            std::cerr << dev.errmsg() << std::endl;
            continue;
        }
        std::cout << "Grabbed: " << node.devnode() << " Type: " << node.type() << std::endl;
        usleep(1'000'000);
        std::cout << "Ungrabbed: " << node.devnode() << std::endl;
    }

    udev_unref(udev);
    return 0;
}
