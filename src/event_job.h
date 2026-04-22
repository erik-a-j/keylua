#ifndef EVENT_JOB_H
#define EVENT_JOB_H

#include <cstdint>
#include <vector>
#include <variant>

struct InputAtom {
    uint16_t type;
    uint16_t code;
    int32_t value;
};

struct AtomSequenceJob {
    std::vector<InputAtom> atoms;
    AtomSequenceJob* next{nullptr};
};
struct LuaFunctionJob {
    int lua_ref;
};
using EventJob = std::variant<AtomSequenceJob, LuaFunctionJob>;

struct EventJobRef {
    uint32_t id;
};

#endif /* #ifndef EVENT_JOB_H */
