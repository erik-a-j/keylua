
#include "device.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <memory>
#include <cstdint>
#include <cstring>
#include <utility>

static const char* find_iface_syspath(
    udev* udev,
    const char* vendor_id,
    const char* product_id,
    const char* protocol
) {
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

    udev_list_entry_foreach(entry, devices) {
        const char* spath = udev_list_entry_get_name(entry);
        udev_device* dev = udev_device_new_from_syspath(udev, spath);
        if (!dev) continue;

        udev_device* parent = udev_device_get_parent_with_subsystem_devtype(
            dev, "usb", "usb_device");
        if (!parent) { udev_device_unref(dev); continue; }

        const char* vid = udev_device_get_property_value(parent, "ID_VENDOR_ID");
        const char* pid = udev_device_get_property_value(parent, "ID_MODEL_ID");

        if (vid && pid && 0 == std::strcmp(vid, vendor_id) && 0 == std::strcmp(pid, product_id)) {
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

static udev_device* find_event_node(udev* udev, const char* iface_syspath) {
    udev_enumerate* enumerate;
    udev_list_entry* devices;
    udev_list_entry* entry;

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(entry, devices) {
        const char* syspath = udev_list_entry_get_name(entry);
        udev_device* dev = udev_device_new_from_syspath(udev, syspath);
        if (!dev) continue;

        const char* devnode = udev_device_get_devnode(dev);
        if (!devnode || 0 != std::strncmp(devnode, "/dev/input/event", 16)) {
            udev_device_unref(dev);
            continue;
        }

        udev_device* parent = udev_device_get_parent(dev);
        while (parent) {
            const char* ppath = udev_device_get_syspath(parent);
            if (ppath && 0 == std::strcmp(ppath, iface_syspath)) {
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

InputInterface::InputInterface(::udev_device* udev_dev, Type type)
    : m_udev_dev{::udev_device_ref(udev_dev), &::udev_device_unref},
    m_type{type} {}

InputInterface::InputInterface(InputInterface&& other)
    : m_udev_dev{std::move(other.m_udev_dev)},
    m_type{other.m_type} {}

std::string_view InputInterface::name() const
{
    const char* name{nullptr};
    ::udev_device* parent = this->udev_parent();
    if (parent) {
        name = ::udev_device_get_sysattr_value(parent, "name");
    }
    return name ? std::string_view{name} : m_unknown;
}
std::string_view InputInterface::devpath() const
{
    const char* devpath = ::udev_device_get_devpath(m_udev_dev.get());
    return devpath ? std::string_view{devpath} : m_unknown;
}
std::string_view InputInterface::devnode() const
{
    const char* devnode = ::udev_device_get_devnode(m_udev_dev.get());
    return devnode ? std::string_view{devnode} : m_unknown;
}

udev_device* InputInterface::udev_parent() const
{
    return ::udev_device_get_parent_with_subsystem_devtype(m_udev_dev.get(), "input", nullptr);
}


Device::Device(::udev* udev, const char* vid, const char* pid)
    : m_vid{vid},
    m_pid{pid}
{
    const char* iface;
    ::udev_device* evnode;
    m_input_interfaces.reserve(2);

    iface = ::find_iface_syspath(udev, vid, pid, "01");
    if (iface) {
        evnode = ::find_event_node(udev, iface);
        if (evnode) {
            m_input_interfaces.emplace_back(evnode, InputInterface::Type::Keyboard);
        }
    }
    iface = ::find_iface_syspath(udev, vid, pid, "02");
    if (iface) {
        evnode = ::find_event_node(udev, iface);
        if (evnode) {
            m_input_interfaces.emplace_back(evnode, InputInterface::Type::Mouse);
        }
    }
}


