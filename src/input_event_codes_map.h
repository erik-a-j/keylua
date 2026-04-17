#ifndef INPUT_EVENT_CODES_MAP_H
#define INPUT_EVENT_CODES_MAP_H

#include <linux/input-event-codes.h>
#include <cstddef>
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
#endif /* #ifndef INPUT_EVENT_CODES_MAP_GPERF */

#endif /* #ifndef INPUT_EVENT_CODES_MAP_H */
