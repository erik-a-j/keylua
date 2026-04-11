/* device_enumerator.cpp BEGIN */

#include "device_enumerator.h"
#include <libevdev/libevdev.h>
#include <fcntl.h>
#include <unistd.h>

template <bool OwnDevices>
std::vector<BaseDeviceInfo<OwnDevices>> enumerate_devices() {
    std::vector<BaseDeviceInfo<OwnDevices>> devices;
    devices.reserve(16);
    libevdev* dev{nullptr};

    for (int i = 0;; ++i) {
        char path[32]{};
        std::snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd == -1) { break; }

        if (0 == libevdev_new_from_fd(fd, &dev)) {
            devices.emplace_back(path, dev, fd);
            if constexpr (!OwnDevices) {
                libevdev_free(dev);
                close(fd);
            }
            dev = nullptr;
        }
        else {
            close(fd);
        }
    }
    return devices;
}

template std::vector<BaseDeviceInfo<true>>  enumerate_devices<true>();
template std::vector<BaseDeviceInfo<false>> enumerate_devices<false>();


/* device_enumerator.cpp END */
