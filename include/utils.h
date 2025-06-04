//
// Created by Romir Kulshrestha on 04/06/2025.
//

#ifndef UTILS_H
#define UTILS_H

#define limbo() for(;;) asm("wfi")

inline void swap(char *a, char *b) {
    const auto t = *a;
    *a = *b;
    *b = t;
}

#endif //UTILS_H
