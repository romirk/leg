//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef LOGS_H
#define LOGS_H

#define LOG_INFO

#ifdef LOG_DEBUG
#define LOG_INFO
#define dbg(fmt, ...) printf("\033[37mdebug | " fmt "\033[0m\n", ##__VA_ARGS__)
#else
#define dbg(...)
#endif

#ifdef LOG_INFO
#define LOG_WARN
#define log(fmt, ...) printf(        " info | " fmt "\n", ##__VA_ARGS__)
#else
#define log(...)
#endif

#ifdef LOG_WARN
#define LOG_ERROR
#define warn(fmt, ...) printf("\033[33m warn | " fmt "\033[0m\n", ##__VA_ARGS__)
#else
#define warn(...)
#endif

#ifdef LOG_ERROR
#define err(fmt, ...) printf("\033[31merror | " fmt "\033[0m\n", ##__VA_ARGS__)
#else
#define err(...)
#endif

#endif //LOGS_H
