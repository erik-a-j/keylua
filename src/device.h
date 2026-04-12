#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <vector>
#include <libevdev/libevdev.h>
#include <libudev.h>

const char* find_event_node(udev* udev, const char* iface_syspath);
const char* find_iface_syspath(
    udev* udev,
    const char* vendor_id,
    const char* product_id,
    const char* protocol
);

class EventNode {
public:
    enum class Type { Keyboard, Mouse };

    EventNode(const char* path, Type type) : m_path{path}, m_type{type} {}

    const std::string& path() const { return m_path; }
    Type type() const { return m_type; }
private:
    std::string m_path;
    Type m_type;
};

class Device {
    std::string m_vendor_id;
    std::string m_product_id;
    std::vector<EventNode> m_event_nodes;

public:
    explicit Device(udev* udev, const char* vendor_id, const char* product_id);

    const std::string& vendor_id() const { return m_vendor_id; }
    const std::string& product_id() const { return m_product_id; }
    const std::vector<EventNode>& event_nodes() const { return m_event_nodes; }
};

/* class DeviceInfo {
     std::string m_path{};
     uint16_t m_product_id{};
     uint16_t m_vendor_id{};
     libevdev* m_dev{nullptr};
     int m_fd{-1};

 public:
     explicit DeviceInfo(const std::string& path) {
         m_fd = ::open(path.c_str(), O_RDONLY|O_NONBLOCK);
         if (m_fd == -1) {
             throw std::runtime_error("Error: " + path + "does not exist");
         }
         int err = ::libevdev_new_from_fd(m_fd, &m_dev);
         if (err != 0) {
             throw std::runtime_error("libevdev_new_from_fd Error: " + std::string{std::strerror(err)});
         }
         m_path = path;
         m_product_id = static_cast<uint16_t>(::libevdev_get_id_product(m_dev));
         m_vendor_id = static_cast<uint16_t>(::libevdev_get_id_vendor(m_dev));
     }

     DeviceInfo(DeviceInfo&& other) noexcept
         : m_path{std::move(other.m_path)},
         m_product_id{other.m_product_id},
         m_vendor_id{other.m_vendor_id},
         m_dev{other.m_dev},
         m_fd{other.m_fd} {
         other.m_dev = nullptr;
         other.m_fd = -1;
     }
     DeviceInfo& operator=(DeviceInfo&&) = delete;
     DeviceInfo(const DeviceInfo&) = delete;
     DeviceInfo& operator=(const DeviceInfo&) = delete;

     ~DeviceInfo() {
         if (m_dev) { ::libevdev_free(m_dev); }
         if (m_fd != -1) { ::close(m_fd); }
     }

     const std::string& path() const { return m_path; }
     uint16_t productID() const { return m_product_id; }
     uint16_t vendorID() const { return m_vendor_id; }
     libevdev* dev() const { return m_dev; }
     int fd() const { return m_fd; }
     bool is_virtual() const {
         const char* phys = ::libevdev_get_phys(m_dev);
         return phys == nullptr || phys[0] == '\0';
     }
 }; */

#endif /* #ifndef DEVICE_H */
