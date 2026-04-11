/* device_enumerator.cpp BEGIN */

#include "device_enumerator.h"
#include <libevdev/libevdev.h>
#include <fcntl.h>
#include <unistd.h>

std::vector<device_info_t> enumerate_devices() {
    std::vector<device_info_t> devices;
    devices.reserve(16);
    libevdev* dev{nullptr};

    for (int i = 0;; ++i) {
        char path[32]{};
        std::snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd == -1) { break; }

        if (0 == libevdev_new_from_fd(fd, &dev)) {
            devices.emplace_back(libevdev_get_id_vendor(dev), libevdev_get_id_product(dev), path);
            libevdev_free(dev);
            dev = nullptr;
        }

        close(fd);
    }
    return devices;
}

/* device_enumerator.cpp END */
