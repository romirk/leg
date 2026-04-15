file(GLOB USR_LIBC_SOURCES "${CMAKE_SOURCE_DIR}/src/libc/*.c")

add_executable(hash.elf
        ${CMAKE_SOURCE_DIR}/src/usr/hash.c
        ${USR_LIBC_SOURCES} ${USER_LINKER_SCRIPT})

add_executable(brot.elf
        ${CMAKE_SOURCE_DIR}/src/usr/mandelbrot.c
        ${USR_LIBC_SOURCES} ${USER_LINKER_SCRIPT})

add_executable(matrix.elf
        ${CMAKE_SOURCE_DIR}/src/usr/matrix.c
        ${USR_LIBC_SOURCES} ${USER_LINKER_SCRIPT})

add_executable(init.elf
        ${CMAKE_SOURCE_DIR}/src/usr/init.c
        ${USR_LIBC_SOURCES} ${USER_LINKER_SCRIPT})

target_link_options(hash.elf PRIVATE -Wl,-T,${USER_LINKER_SCRIPT})
target_link_options(brot.elf PRIVATE -Wl,-T,${USER_LINKER_SCRIPT})
target_link_options(matrix.elf PRIVATE -Wl,-T,${USER_LINKER_SCRIPT})
target_link_options(init.elf PRIVATE -Wl,-T,${USER_LINKER_SCRIPT})
