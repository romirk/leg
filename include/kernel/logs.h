#ifndef LOGS_H
#define LOGS_H

// stringify macros
#define STR_HELPER(X) #X
#define STR(X)        STR_HELPER(X)

#ifdef LOG_DEBUG
#define LOG_INFO
#ifndef NO_ANSI
#define dbg(fmt, ...)                                                                              \
    (printf("\033[2;37m debug \033[2;90;47m %s:%d \033[0;37;49m ", __FILE_NAME__, __LINE__),       \
     printf(fmt "\033[0m\n", ##__VA_ARGS__))
#else
#define dbg(fmt, ...)                                                                              \
    (printf("[ DEBUG ] %s:%d | ", __FILE_NAME__, __LINE__), printf(fmt "\n", ##__VA_ARGS__))
#endif
#else
#define dbg(...)
#endif

#ifdef LOG_INFO
#define LOG_WARN
#ifndef NO_ANSI
#define info(fmt, ...) (print("\033[97m  info \033[0m "), printf(fmt "\n", ##__VA_ARGS__))
#else
#define info(fmt, ...) (printf("[  INFO ] "), printf(fmt "\n", ##__VA_ARGS__))
#endif
#else
#define info(...)
#endif

#ifdef LOG_WARN
#define LOG_ERROR
#define warn(fmt, ...)                                                                             \
    (printf("\033[97;103m  warn \033[93;107m %s:%d \033[33;49m ", __FILE_NAME__, __LINE__),        \
     printf(fmt "\033[0m\n", ##__VA_ARGS__))
#else
#define warn(...)
#endif

#ifdef LOG_ERROR
// LOG_ERROR is defined if any log level is active, so ensure that stdio is included.
#include "libc/stdio.h"

#ifndef NO_ANSI
#define err(fmt, ...)                                                                              \
    (printf("\033[97;101m error \033[91;107m %s:%d \033[31;49m ", __FILE_NAME__, __LINE__),        \
     printf(fmt "\033[0m\n", ##__VA_ARGS__))
#else
#define err(fmt, ...)                                                                              \
    (printf("[ ERROR ] %s:%d | ", __FILE_NAME__, __LINE__), printf(fmt "\n", ##__VA_ARGS__))
#endif

#else
#define err(...)
#endif

#endif // LOGS_H
