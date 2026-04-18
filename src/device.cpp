
#include "device.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <memory>
#include <format>
#include <cstdint>
#include <cstring>
#include <utility>

static inline void vidpidstr(char(&buf)[5], uint16_t num)
{
    buf[4] = '\0';
    for (int i = 3; i >= 0; --i)
    {
        buf[i] = num % 0x10;
        buf[i] += (buf[i] > 0x09) ? 'a' - 0x0a : '0';
        num /= 0x10;
    }
}

static const char* find_iface_syspath(
    udev* udev,
    const char* vendor_id,
    const char* product_id,
    const char* protocol
)
{
    static char syspath[512];
    udev_enumerate* enumerate;
    udev_list_entry* devices;
    udev_list_entry* entry;

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_add_match_sysattr(enumerate, "bInterfaceClass", "03");
    udev_enumerate_add_match_sysattr(enumerate, "bInterfaceProtocol", protocol);
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(entry, devices)
    {
        const char* spath = udev_list_entry_get_name(entry);
        udev_device* dev = udev_device_new_from_syspath(udev, spath);
        if (!dev) continue;

        udev_device* parent = udev_device_get_parent_with_subsystem_devtype(
            dev, "usb", "usb_device");
        if (!parent) { udev_device_unref(dev); continue; }

        const char* vid = udev_device_get_property_value(parent, "ID_VENDOR_ID");
        const char* pid = udev_device_get_property_value(parent, "ID_MODEL_ID");

        if (vid && pid && 0 == std::strcmp(vid, vendor_id) && 0 == std::strcmp(pid, product_id))
        {
            std::strncpy(syspath, spath, sizeof(syspath) - 1);
            udev_device_unref(dev);
            udev_enumerate_unref(enumerate);
            return syspath;
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    return NULL;
}

static udev_device* find_event_node(udev* udev, const char* iface_syspath)
{
    udev_enumerate* enumerate;
    udev_list_entry* devices;
    udev_list_entry* entry;

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(entry, devices)
    {
        const char* syspath = udev_list_entry_get_name(entry);
        udev_device* dev = udev_device_new_from_syspath(udev, syspath);
        if (!dev) continue;

        const char* devnode = udev_device_get_devnode(dev);
        if (!devnode || 0 != std::strncmp(devnode, "/dev/input/event", 16))
        {
            udev_device_unref(dev);
            continue;
        }

        udev_device* parent = udev_device_get_parent(dev);
        while (parent)
        {
            const char* ppath = udev_device_get_syspath(parent);
            if (ppath && 0 == std::strcmp(ppath, iface_syspath))
            {
                udev_device* result = udev_device_ref(dev);
                udev_device_unref(dev);
                udev_enumerate_unref(enumerate);
                return result;
            }
            parent = udev_device_get_parent(parent);
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    return NULL;
}

Device::Device(::udev* udev, uint16_t vid, uint16_t pid, DeviceType type)
    : m_vid{vid},
    m_pid{pid},
    m_type{type}
{
    const char* iface_type;
    const char* iface;
    ::udev_device* evnode;

    char svid[5];
    char spid[5];
    vidpidstr(svid, vid);
    vidpidstr(spid, pid);

    if (type == DeviceType::Keyboard) { iface_type = "01"; }
    else if (type == DeviceType::Mouse) { iface_type = "02"; }

    iface = ::find_iface_syspath(udev, svid, spid, iface_type);
    if (iface)
    {
        evnode = ::find_event_node(udev, iface);
        if (evnode)
        {
            m_udev_dev.reset(::udev_device_ref(evnode));
        }
    }
}

std::string_view Device::name() const
{
    const char* name{nullptr};
    ::udev_device* parent = this->udev_parent();
    if (parent) { name = ::udev_device_get_sysattr_value(parent, "name"); }
    return name ? std::string_view{name} : m_unknown;
}
std::string_view Device::devpath() const
{
    const char* devpath{nullptr};
    ::udev_device* dev = m_udev_dev.get();
    if (dev) { devpath = ::udev_device_get_devpath(dev); }
    return devpath ? std::string_view{devpath} : m_unknown;
}
std::string_view Device::devnode() const
{
    const char* devnode{nullptr};
    ::udev_device* dev = m_udev_dev.get();
    if (dev) { devnode = ::udev_device_get_devnode(dev); }
    return devnode ? std::string_view{devnode} : m_unknown;
}
udev_device* Device::udev_parent() const
{
    ::udev_device* dev = m_udev_dev.get();
    return dev ? ::udev_device_get_parent_with_subsystem_devtype(dev, "input", nullptr) : nullptr;
}
