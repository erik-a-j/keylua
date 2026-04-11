/* main.cpp BEGIN */

#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <string>
#include <unistd.h>
#include "device_enumerator.h"
#include "device_grabber.h"

bool is_mouse(libevdev* dev) {
    return libevdev_has_event_type(dev, EV_REL) &&
        libevdev_has_event_code(dev, EV_REL, REL_X) &&
        libevdev_has_event_code(dev, EV_REL, REL_Y) &&
        libevdev_has_event_code(dev, EV_KEY, BTN_LEFT);
}

bool is_keyboard(libevdev* dev) {
    return libevdev_has_event_type(dev, EV_KEY) &&
        libevdev_has_event_code(dev, EV_KEY, KEY_A) &&
        libevdev_has_event_code(dev, EV_KEY, KEY_SPACE) &&
        !libevdev_has_event_type(dev, EV_REL);
}

void print_device_info(const device_info_t& d) {
    std::cout << "- path: " << d.path << " product:vendor: " << std::hex << d.product_id << ':' << d.vendor_id << std::dec << std::endl;
}

int main() {
    auto devices = enumerate_devices();
    for (const auto& d : devices) {
        print_device_info(d);
        if (d.product_id == 0x0015 && d.vendor_id == 0x1532) {
            std::cout << "Found Naga\n";
        }
    }


    return 0;
}

/* main.cpp END */
