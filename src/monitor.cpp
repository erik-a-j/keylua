
#include "monitor.h"
#include <libevdev/libevdev.h>
#include <iostream>
#include <string>

void monitor_event_callback(uint32_t device_id, const input_event* ev, void* usr_data)
{
    if (ev->type != EV_KEY) return;

    usr_data_t* x = reinterpret_cast<usr_data_t*>(usr_data);
    const auto& d = x->devices[device_id];

    auto value_to_action = [](int32_t v) -> const char*
    {
        switch (v)
        {
        case 0: return "RELEASE";
        case 1: return "PRESS";
        case 2: return "REPEAT";
        default: return nullptr;
        }
    };

    const char* action = value_to_action(ev->value);
    const char* key = libevdev_event_code_get_name(ev->type, ev->code);

    std::string map = "passthrough";
    {
        auto it = d.mappings.find(ev->code);
        if (it != d.mappings.end())
        {
            const std::optional<uint32_t>& slot = it->second[static_cast<size_t>(ev->value)];
            if (slot.has_value())
            {
                const EventJob& job = x->jobs[slot.value()];
                std::visit([&](const auto& j)
                {
                    using jT = std::decay_t<decltype(j)>;

                    if constexpr (std::is_same_v<jT, AtomSequenceJob>)
                    {
                        map = "AtomSequenceJob{";
                        for (const auto& atom : j.atoms)
                        {
                            map += "\n  ";
                            std::visit([&](const auto& a)
                            {
                                using aT = std::decay_t<decltype(a)>;

                                if constexpr (std::is_same_v<aT, InputAtom>)
                                {
                                    const char* seqaction = value_to_action(a.value);
                                    const char* seqkey = libevdev_event_code_get_name(a.type, a.code);
                                    map += "InputAtom{";
                                    map += seqaction ? seqaction : "value(" + std::to_string(a.value) + ")";
                                    map += ": ";
                                    map += seqkey ? seqkey : "value(" + std::to_string(a.code) + ")";
                                }
                                else if constexpr (std::is_same_v<aT, SleepAtom>)
                                {
                                    map += "SleepAtom{";
                                    map += std::to_string(a.ms) + "ms";
                                }
                            }, atom);
                            map += "},";
                        }
                        map += "\n}";
                    }
                    else if constexpr (std::is_same_v<jT, LuaFunctionJob>)
                    {
                        map = "LuaFunctionJob";
                    }
                }, job);
            }
        }

        std::cout << d.name << ": action=";
        if (action) std::cout << action;
        else std::cout << "value(" << ev->value << ")";

        std::cout << ", key=";
        if (key) std::cout << key;
        else std::cout << "value(" << ev->code << ")";

        std::cout << ", map=" << map << std::endl;
    }
}