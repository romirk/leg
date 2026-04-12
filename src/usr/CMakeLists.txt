file(GLOB USR_LIBC_SOURCES "${CMAKE_SOURCE_DIR}/src/libc/*.c")
list(REMOVE_ITEM USR_LIBC_SOURCES "${CMAKE_SOURCE_DIR}/src/libc/vec.c")

file(GLOB USR_SOURCES "*.c")

add_executable(user.elf ${USR_SOURCES} ${USR_LIBC_SOURCES} ${USER_LINKER_SCRIPT})
target_link_options(user.elf PRIVATE -Wl,-T,${USER_LINKER_SCRIPT})