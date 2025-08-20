//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef LOGS_H
#define LOGS_H

// stringify macros
// #define __STR_HELPER(X) #X
// #define STR(X) __STR_HELPER(X)
// #define __LOC__ __FILE_NAME__ ":" STR(__LINE__)

#ifdef LOG_DEBUG
#define LOG_INFO
#define dbg(fmt, ...) (printf("\033[2;37m debug \033[2;90;47m %s:%d \033[0;37;49m ", __FILE_NAME__, __LINE__), printf(fmt "\033[0m\n", ##__VA_ARGS__))
#else
#define dbg(...)
#endif

#ifdef LOG_INFO
#define LOG_WARN
#define info(fmt, ...) (print("\033[97m  info \033[0m "), printf(fmt "\n", ##__VA_ARGS__))
#else
#define info(...)
#endif

#ifdef LOG_WARN
#define LOG_ERROR
#define warn(fmt, ...) (printf("\033[97;103m  warn \033[93;107m %s:%d \033[33;49m ", __FILE_NAME__, __LINE__), printf(fmt "\033[0m\n", ##__VA_ARGS__))
#else
#define warn(...)
#endif

#ifdef LOG_ERROR
#define err(fmt, ...) (printf("\033[97;101m error \033[91;107m %s:%d \033[31;49m ", __FILE_NAME__, __LINE__), printf(fmt "\033[0m\n", ##__VA_ARGS__))

// LOG_ERROR is defined if any log level is active, so ensure that stdio is included.
#include "stdio.h"

#else
#define err(...)
#endif

#endif //LOGS_H