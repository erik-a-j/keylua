#ifndef EVENT_JOB_H
#define EVENT_JOB_H

#include <cstdint>
#include <vector>

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

#endif /* #ifndef EVENT_JOB_H */
