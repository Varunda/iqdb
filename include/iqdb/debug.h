#ifndef IQDB_DEBUG_H
#define IQDB_DEBUG_H

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <chrono>

namespace iqdb {

// The logging verbosity level. 0 = DEBUG, 1 = ERROR, 2 = WARN, 3 = INFO.
extern int debug_level;

// The backtrace of the most recently thrown exception. Set in __cxa_throw
// every time an exception is thrown.
extern thread_local std::string last_exception_backtrace;

// Generate a pretty-printed stack trace. `skip` skips the last N callers.
std::string get_backtrace(int skip = 1);

// Demangle a C++ symbol name.
std::string demangle_name(std::string symbol_name);

template<typename... Args>
inline void LOG(std::string format, int level, Args... args) {
    if (level >= debug_level) {
        auto now = std::chrono::system_clock::now();
        fmt::print(stderr, "{:%FT%H:%M:%S} ", now);
        fmt::print(stderr, format, args...); \
    }
}

template<typename... Args>
inline void DEBUG(std::string format, Args... args) {
  LOG("[debug] " + format, 0, args...);
}

template<typename... Args>
inline void ERROR(std::string format, Args... args) {
  LOG("[error] " + format, 1, args...);
}

template<typename... Args>
inline void WARN(std::string format, Args... args) {
  LOG("[warn] " + format, 2, args...);
}

template<typename... Args>
inline void INFO(std::string format, Args... args) {
  LOG("[info] " + format, 3, args...);
}

}

#endif // IQDB_DEBUG_H
