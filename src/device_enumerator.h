#ifndef DEVICE_ENUMERATOR_H
#define DEVICE_ENUMERATOR_H

#include <vector>
#include "device.h"

std::vector<device_info_t> enumerate_devices(bool keep_devices);

#endif /* #ifndef DEVICE_ENUMERATOR_H */
