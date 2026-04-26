#ifndef DEVICE_H
#define DEVICE_H

#include <cstdint>
#include <string>
#include <string_view>

struct sd_device;
struct sd_device_enumerator;

class Device {
public:
    explicit Device(uint16_t vid, uint16_t pid);
    //explicit Device(const char* devname);

    Device(Device&& other) = delete;
    Device& operator=(Device&&) = delete;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    ~Device();

    bool has_keyboard() const { return m_keyboard != nullptr; }
    bool has_mouse() const { return m_mouse != nullptr; }
    std::string keyboard_devname() const;
    std::string mouse_devname() const;

    explicit operator bool() const { return m_dev != nullptr && m_errbuf.empty(); }
    std::string_view errmsg() const { return m_errbuf; }

private:
    sd_device* usb_from_vid_pid(uint16_t vid, uint16_t pid);
    sd_device* usb_iface(sd_device_enumerator** en);

    sd_device* m_dev{nullptr};
    sd_device* m_keyboard{nullptr};
    sd_device* m_mouse{nullptr};
    int m_protocols{0};
    std::string m_errbuf;

    //static constexpr std::string_view m_typestr[2]{"keyboard", "mouse"};
    //static constexpr std::string_view m_unknown{"<unknown>"};
};

#endif /* #ifndef DEVICE_H */
