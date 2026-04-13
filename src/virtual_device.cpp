
#include "virtual_device.h"
#include <libevdev/libevdev-uinput.h>
#include <cstring>

VirtualDevice::VirtualDevice(const ::libevdev* src)
    : m_uinput_dev{nullptr}
{
    int err = ::libevdev_uinput_create_from_device(src, LIBEVDEV_UINPUT_OPEN_MANAGED, &m_uinput_dev);
    if (err != 0)
    {
        m_errbuf = "libevdev_uinput_create_from_device Error: " + std::string{std::strerror(-err)};
        return;
    }
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

bool VirtualDevice::emit(const ::input_event& ev)
{
    int err = ::libevdev_uinput_write_event(m_uinput_dev, ev.type, ev.code, ev.value);
    if (err != 0)
    {
        m_errbuf = "libevdev_uinput_write_event Error: " + std::string{std::strerror(-err)};
    }
    return err == 0;
}