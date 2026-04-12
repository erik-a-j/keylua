/* main.cpp BEGIN */

#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <string>
#include <unistd.h>
#include "device.h"

void print_device_info(const Device& d) {
    std::cout << "- Product:Vendor: " << d.pid() << ':' << d.vid() << '\n'
        << "  - Event Nodes:\n";
    for (const auto& node : d.evnodes()) {
        std::cout << "    - Path: " << node.path() << '\n'
            << "      Type: " << node.type() << '\n';
    }
}

int main() {
    udev* udev = udev_new();

    const char* vid = "1532";
    const char* pid = "0015";
    Device d{udev, vid, pid};
    print_device_info(d);

    std::cout << std::endl;

    /*     for (const auto& node : d.evnodes()) {
             DeviceGrabber dev{node};
             if (!dev) {
                 std::cerr << dev.errmsg() << std::endl;
                 continue;
             }
             std::cout << "Grabbed: " << node.path() << " Type: " << node.type() << std::endl;
             usleep(1'000'000);
             std::cout << "Ungrabbed: " << node.path() << std::endl;
         } */

    udev_unref(udev);
    return 0;
}

/* main.cpp END */
