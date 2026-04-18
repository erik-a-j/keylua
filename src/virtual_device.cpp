
#include "virtual_device.h"
#include <libevdev/libevdev-uinput.h>
#include <cstring>

VirtualDevice::VirtualDevice(const ::libevdev* src)
{
    ::libevdev* tmpl = ::libevdev_new();

    ::libevdev_set_name(tmpl, "keylua virtual device");
    ::libevdev_set_id_vendor(tmpl, 0x0000);
    ::libevdev_set_id_product(tmpl, 0x0000);
    ::libevdev_set_id_version(tmpl, 1);
    ::libevdev_set_id_bustype(tmpl, BUS_VIRTUAL);

    for (unsigned int type = 0; type < static_cast<unsigned int>(EV_CNT); type++)
    {
        if (!::libevdev_has_event_type(src, type)) continue;
        ::libevdev_enable_event_type(tmpl, type);
        for (unsigned int code = 0; code < static_cast<unsigned int>(::libevdev_event_type_get_max(type)); code++)
        {
            if (!::libevdev_has_event_code(src, type, code)) continue;
            if (type == EV_ABS)
            {
                const input_absinfo* abs = ::libevdev_get_abs_info(src, code);
                ::libevdev_enable_event_code(tmpl, type, code, abs);
            }
            else
            {
                ::libevdev_enable_event_code(tmpl, type, code, nullptr);
            }
        }
    }

    int err = ::libevdev_uinput_create_from_device(tmpl, LIBEVDEV_UINPUT_OPEN_MANAGED, &m_uinput_dev);
    ::libevdev_free(tmpl);

    if (err != 0)
    {
        m_errbuf = "libevdev_uinput_create_from_device Error: " + std::string{std::strerror(-err)};
    }
}
VirtualDevice::VirtualDevice(VirtualDevice&& other)
    : m_uinput_dev{other.m_uinput_dev},
    m_errbuf{other.m_errbuf}
{
    other.m_uinput_dev = nullptr;
}
VirtualDevice& VirtualDevice::operator=(VirtualDevice&& other)
{
    if (this != &other)
    {
        m_uinput_dev = other.m_uinput_dev;
        other.m_uinput_dev = nullptr;
        m_errbuf = other.m_errbuf;
    }
    return *this;
}
VirtualDevice::~VirtualDevice()
{
    this->close();
}
void VirtualDevice::close()
{
    if (m_uinput_dev != nullptr)
    {
        ::libevdev_uinput_destroy(m_uinput_dev);
    }
}

bool VirtualDevice::emit(uint16_t type, uint16_t code, int32_t value)
{

    int err = ::libevdev_uinput_write_event(m_uinput_dev, type, code, value);
    if (err != 0)
    {
        m_errbuf += "\nlibevdev_uinput_write_event Error: " + std::string{std::strerror(-err)};
    }
    return err == 0;
}