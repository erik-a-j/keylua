#ifndef DEVICE_ENUMERATOR_H
#define DEVICE_ENUMERATOR_H

#include <vector>
#include "device.h"

template <bool OwnDevices>
std::vector<BaseDeviceInfo<OwnDevices>> enumerate_devices();

extern template std::vector<BaseDeviceInfo<true>>  enumerate_devices<true>();
extern template std::vector<BaseDeviceInfo<false>> enumerate_devices<false>();

#endif /* #ifndef DEVICE_ENUMERATOR_H */
