#ifndef INPUT_EVENT_CODES_MAP_H
#define INPUT_EVENT_CODES_MAP_H

#include <linux/input-event-codes.h>
#include <cstddef>
#include <cstdint>
#include <cstring>

struct keyword {
    const char* key;
    int code;
};

#ifndef INPUT_EVENT_CODES_MAP_GPERF
class InputEventCodesMap
{
private:
    static inline unsigned int hash(const char* str, size_t len);
public:
    static const struct keyword* evcode(const char* str, size_t len);
};

inline const struct keyword* symbol_to_evcode(const char* str)
{
    return InputEventCodesMap::evcode(str, std::strlen(str));
}
/* inline const uint16_t* symbol_to_evcode(const char* str)
 {
     return reinterpret_cast<const uint16_t*>(&InputEventCodesMap::evcode(str, std::strlen(str))->code);
 } */
#endif /* #ifndef INPUT_EVENT_CODES_MAP_GPERF */


#endif /* #ifndef INPUT_EVENT_CODES_MAP_H */
