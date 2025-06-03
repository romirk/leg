//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef LOGS_H
#define LOGS_H

#define LOG_INFO

// stringify macros
#define __STR(X) #X
#define STR(X) __STR(X)
#define __LOC__ __FILE_NAME__ ":" STR(__LINE__)

#ifdef LOG_DEBUG
#define LOG_INFO
#define dbg(fmt, ...) printf("\033[2;37m debug \033[2;90;47m " __LOC__ " \033[0;37;49m " fmt "\033[0m\n", ##__VA_ARGS__)
#else
#define dbg(...)
#endif

#ifdef LOG_INFO
#define LOG_WARN
#define log(fmt, ...) printf("\033[97m  info \033[0m " fmt "\n", ##__VA_ARGS__)
#else
#define log(...)
#endif

#ifdef LOG_WARN
#define LOG_ERROR
#define warn(fmt, ...) printf("\033[97;103m  warn \033[93;107m " __LOC__ " \033[33;49m " fmt "\033[0m\n", ##__VA_ARGS__)
#else
#define warn(...)
#endif

#ifdef LOG_ERROR
#define err(fmt, ...) printf("\033[97;101m error \033[91;107m " __LOC__ " \033[31;49m " fmt "\033[0m\n", ##__VA_ARGS__)

// LOG_ERROR is defined if any log level is active, so ensure that stdio is included.
#include "stdio.h"

#else
#define err(...)
#endif

#endif //LOGS_H
