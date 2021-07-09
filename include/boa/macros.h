#ifndef BOA_MACROS_H
#define BOA_MACROS_H

#include <fmt/color.h>

#define REMOVE_COPY_AND_ASSIGN(Class)           \
    Class(Class &) = delete;                    \
    Class &operator=(const Class &) = delete;

#define TODO() do { throw std::runtime_error("Reached TODO"); } while (false)

#ifdef NDEBUG
#   define LOG_INFO(...)
#   define LOG_WARN(...)
#   define LOG_FAIL(...)
#else
#   define LOG_INFO(fmt_, ...)                                  \
do {                                                            \
    fmt::print(fmt::fg(fmt::color::gray), "INFO: ");            \
    fmt::print("{}\n", fmt::format((fmt_), ##__VA_ARGS__));     \
} while (false)
#   define LOG_WARN(fmt_, ...)                                  \
do {                                                            \
    fmt::print(fmt::fg(fmt::color::white), "WARN: ");           \
    fmt::print("{}\n", fmt::format((fmt_), ##__VA_ARGS__));     \
} while (false)
#   define LOG_FAIL(fmt_, ...)                                  \
do {                                                            \
    fmt::print(fmt::fg(fmt::color::red), "FAIL: ");             \
    fmt::print("{}\n", fmt::format((fmt_), ##__VA_ARGS__));     \
} while (false)
#endif

#endif