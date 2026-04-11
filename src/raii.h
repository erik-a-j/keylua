#ifndef RAII_H
#define RAII_H

#include <libevdev/libevdev.h>

namespace raii {

class libevdev {
    ::libevdev* m_value{nullptr};

public:
    libevdev(::libevdev* value) : m_value{value} {}
    ~libevdev() {
        if (m_value != nullptr) {
            ::libevdev_free(m_value);
        }
    }
    ::libevdev* operator()() { return m_value; }
    const ::libevdev* operator()() const { return m_value; }
};

} // namespace raii

#endif /* #ifndef RAII_H */
