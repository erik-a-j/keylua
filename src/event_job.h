#ifndef EVENT_JOB_H
#define EVENT_JOB_H

#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>
#include <optional>

struct InputAtom {
    uint16_t type;
    uint16_t code;
    int32_t value;
};

struct EventJob {
    std::vector<InputAtom> atoms;
};

struct EventJobRef {
    uint32_t id;
};

using EventJobMap = std::unordered_map<uint16_t, std::array<std::optional<uint32_t>, 3>>;

#endif /* #ifndef EVENT_JOB_H */
