#ifndef UTILS_H
#define UTILS_H

template <typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

#endif /* #ifndef UTILS_H */
