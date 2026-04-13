#ifndef LOGS_H
#define LOGS_H

// Kernel-only print functions — write directly to UART, bypass tty/fb.
int  kprint(const char *s);
void kprintf(const char *fmt, ...);

// stringify macros
#define STR_HELPER(X) #X
#define STR(X)        STR_HELPER(X)

#ifdef LOG_DEBUG
#define LOG_INFO
#define dbg(fmt, ...)                                                                              \
    (kprintf("\033[2;37m debug \033[2;90;47m %s:%d \033[0;37;49m ", __FILE_NAME__, __LINE__),      \
     kprintf(fmt "\033[0m\n", ##__VA_ARGS__))
#else
#define dbg(...)
#endif

#ifdef LOG_INFO
#define LOG_WARN
#define info(fmt, ...) (kprint("\033[97m  info \033[0m "), kprintf(fmt "\n", ##__VA_ARGS__))
#else
#define info(...)
#endif

#ifdef LOG_WARN
#define LOG_ERROR
#define warn(fmt, ...)                                                                             \
    (kprintf("\033[97;103m  warn \033[93;107m %s:%d \033[33;49m ", __FILE_NAME__, __LINE__),       \
     kprintf(fmt "\033[0m\n", ##__VA_ARGS__))
#else
#define warn(...)
#endif

#ifdef LOG_ERROR
#define err(fmt, ...)                                                                              \
    (kprintf("\033[97;101m error \033[91;107m %s:%d \033[31;49m ", __FILE_NAME__, __LINE__),       \
     kprintf(fmt "\033[0m\n", ##__VA_ARGS__))
#else
#define err(...)
#endif

#endif // LOGS_H