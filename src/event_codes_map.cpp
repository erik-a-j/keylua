#include "event_codes_map.h"

const struct keyword* EventCodesMap::lookup(const char* str, size_t len)
{
    const struct keyword* kw = EventCodesMap::lookup_code(str, len);
    if (kw) return kw;
    return EventCodesMap::lookup_alias(str, len);
}

std::unordered_map<uint16_t, const char*> EventCodesMap::rlookup_code()
{
    std::unordered_map<uint16_t, const char*> rmap;
    for (size_t i = 0; i <= EventCodesMap::MAX_HASH_VALUE_CODE; ++i)
    {
        const auto& kw = EventCodesMap::WORDLIST_CODE[i];
        if (kw.code != -1)
        {
            rmap[kw.code] = kw.key;
        }
    }
    return rmap;
}