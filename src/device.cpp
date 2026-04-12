/* device.cpp BEGIN */

#include "device.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <memory>
#include <cstdint>
#include <cstring>
#include <utility>

const char* find_iface_syspath(
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

const char* find_event_node(udev* udev, const char* iface_syspath) {
    static char node[256];
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
                std::strncpy(node, devnode, sizeof(node) - 1);
                udev_device_unref(dev);
                udev_enumerate_unref(enumerate);
                return node;
            }
            parent = udev_device_get_parent(parent);
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    return NULL;
}


Device::Device(udev* udev, const char* vendor_id, const char* product_id)
    : m_vendor_id{vendor_id},
    m_product_id{product_id} {

    const char* iface;
    const char* evnode;

    iface = find_iface_syspath(udev, vendor_id, product_id, "01");
    if (iface) {
        evnode = find_event_node(udev, iface);
        if (evnode) {
            m_event_nodes.emplace_back(evnode, EventNode::Type::Keyboard);
        }
    }
    iface = find_iface_syspath(udev, vendor_id, product_id, "02");
    if (iface) {
        evnode = find_event_node(udev, iface);
        if (evnode) {
            m_event_nodes.emplace_back(evnode, EventNode::Type::Mouse);
        }
    }
}


/* device.cpp END */


