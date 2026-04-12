/* main.cpp BEGIN */

#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <string>
#include <unistd.h>
#include "device.h"

void print_device_info(const Device& d) {
    std::cout << "- Product:Vendor: " << d.product_id() << ':' << d.vendor_id() << '\n'
        << "  - Event Nodes:\n";
    for (const auto& node : d.event_nodes()) {
        std::cout << "    - Type: " << (node.type() == EventNode::Type::Keyboard ? "Keyboard" : "Mouse") << '\n'
            << "    - Path: " << node.path() << '\n';
    }
}

int main() {
    udev* udev = udev_new();
    const char* vendor_id = "1532";
    const char* product_id = "0015";
    Device d{udev, vendor_id, product_id};
    print_device_info(d);
    udev_unref(udev);
    return 0;
}

/* main.cpp END */
