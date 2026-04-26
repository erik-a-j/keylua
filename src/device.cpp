
#include "device.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <memory>
#include <format>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>
#include <systemd/sd-device.h>

struct DeviceEnumeration {
    template <typename F>
    DeviceEnumeration(F init) { init(m_en); }
    ~DeviceEnumeration() { if (m_en) sd_device_enumerator_unref(m_en); }

    struct Iterator {
        sd_device_enumerator* en_iter;
        sd_device* dev_iter;

        Iterator(sd_device_enumerator* en_iter_, sd_device* dev_iter_)
            : en_iter{en_iter_}, dev_iter{dev_iter_}
        {}

        sd_device* operator*() { return dev_iter; }
        Iterator& operator++()
        {
            dev_iter = sd_device_enumerator_get_device_next(en_iter);
            return *this;
        }
        bool operator!=(const Iterator& o) const { return dev_iter != o.dev_iter; }
    };

    Iterator begin() { return {m_en, sd_device_enumerator_get_device_first(m_en)}; }
    Iterator end() { return {m_en, nullptr}; }

private:
    sd_device_enumerator* m_en{nullptr};
};

Device::Device(uint16_t vid, uint16_t pid)
    : m_dev{usb_from_vid_pid(vid, pid)}
{
    if (!m_dev) { return; }

    DeviceEnumeration iface_en([&](auto& en) {
        sd_device_enumerator_new(&en);
        sd_device_enumerator_add_match_subsystem(en, "usb", true);
        sd_device_enumerator_add_match_property(en, "DEVTYPE", "usb_interface");
        sd_device_enumerator_add_match_parent(en, m_dev);
        sd_device_enumerator_add_match_sysattr(en, "bInterfaceClass", "03", true);
    });
    for (sd_device* iface : iface_en)
    {
        const char* pcol{nullptr};
        sd_device_get_sysattr_value(iface, "bInterfaceProtocol", &pcol);
        if (pcol && pcol[0] == '0' && (pcol[1] == '1' || pcol[1] == '2') && pcol[2] == '\0')
        {
            DeviceEnumeration input_en([&](auto& en) {
                sd_device_enumerator_new(&en);
                sd_device_enumerator_add_match_subsystem(en, "input", true);
                sd_device_enumerator_add_match_parent(en, iface);
            });
            for (sd_device* input : input_en)
            {
                const char* devname{nullptr};
                sd_device_get_devname(input, &devname);
                if (devname && 0 == std::strncmp(devname, "/dev/input/event", 16))
                {
                    sd_device_ref(input);
                    if (pcol[1] == '1') { m_keyboard = input; }
                    else { m_mouse = input; }
                    break;
                }
            }
        }
    }
}

Device::~Device()
{
    if (m_dev) { sd_device_unref(m_dev); }
    if (m_keyboard) { sd_device_unref(m_keyboard); }
    if (m_mouse) { sd_device_unref(m_mouse); }
}

std::string Device::keyboard_devname() const
{
    const char* devname{nullptr};
    if (m_keyboard) { sd_device_get_devname(m_keyboard, &devname); }
    return devname ? std::string{devname} : std::string{};
}
std::string Device::mouse_devname() const
{
    const char* devname{nullptr};
    if (m_mouse) { sd_device_get_devname(m_mouse, &devname); }
    return devname ? std::string{devname} : std::string{};
}

sd_device* Device::usb_from_vid_pid(uint16_t vid, uint16_t pid)
{
    std::string svid = std::format("{:04x}", vid);
    std::string spid = std::format("{:04x}", pid);

    sd_device_enumerator* en = nullptr;
    sd_device_enumerator_new(&en);

    sd_device_enumerator_add_match_subsystem(en, "usb", true);
    sd_device_enumerator_add_match_sysattr(en, "idVendor", svid.c_str(), true);
    sd_device_enumerator_add_match_sysattr(en, "idProduct", spid.c_str(), true);

    sd_device* dev = sd_device_enumerator_get_device_first(en);
    if (!dev)
    {
        m_errbuf = std::string("no device: ") + svid + ":" + spid;
    }
    else
    {
        sd_device_ref(dev);
    }

    sd_device_enumerator_unref(en);
    return dev;                          // caller: sd_device_unref()
}
