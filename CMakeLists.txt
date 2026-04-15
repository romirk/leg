cmake_minimum_required(VERSION 4.0)

set(MCPU cortex-a15)

# project settings
project(leg C ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(LINKER_SCRIPT      ${CMAKE_SOURCE_DIR}/linker.ld)
set(USER_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/src/usr/linker_user.ld)

add_compile_options(-mfloat-abi=hard -mfpu=vfpv4)
add_compile_options(-mcpu=${MCPU} -Wall -Wextra --target=${TARGET_ARCH} -ffreestanding -fbuiltin -flto)
# Link options shared by both targets (no linker script here — set per-target)
add_link_options(-mfloat-abi=hard -mfpu=vfpv4 -flto -fuse-ld=lld --target=${TARGET_ARCH} -mcpu=${MCPU} -nostdlib)

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Wno-unused-command-line-argument")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_definitions(LOG_WARN)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_definitions(LOG_DEBUG)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -fno-slp-vectorize)
    add_compile_definitions(LOG_DEBUG)
endif ()

include_directories(${CMAKE_SOURCE_DIR}/include)

# ── User binary ───────────────────────────────────────────────────────────────

add_subdirectory(src/usr)

set(USER_ELF  ${CMAKE_CURRENT_BINARY_DIR}/src/usr/user.elf)
set(DISK_IMG  ${CMAKE_SOURCE_DIR}/data/disk.img)

add_custom_target(write_disk
    COMMAND ${CMAKE_COMMAND}
            -DELF=${USER_ELF}
            -DDISK=${DISK_IMG}
            -DMKFS=${CMAKE_SOURCE_DIR}/legfs/mkfs.py
            -P ${CMAKE_SOURCE_DIR}/cmake/write_disk.cmake
    DEPENDS ${USER_ELF}
    COMMENT "Packing user.bin into LEGF filesystem image"
)

# ── Kernel binary ─────────────────────────────────────────────────────────────

file(GLOB_RECURSE KERNEL_SOURCES "src/kernel/*.*")
file(GLOB_RECURSE LIBC_SOURCES   "src/libc/*.c")



add_executable(${PROJECT_NAME}.elf
    ${KERNEL_SOURCES}
    ${LIBC_SOURCES}
    ${LINKER_SCRIPT}
)
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE KERNEL)
add_dependencies(${PROJECT_NAME}.elf write_disk)
target_link_options(${PROJECT_NAME}.elf PRIVATE -Wl,-T,${LINKER_SCRIPT})

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE FORMAT_SOURCES
        "src/*.c" "src/*.h"
        "include/*.h"
    )
    list(FILTER FORMAT_SOURCES EXCLUDE REGEX ".*/fonts/.*")
    add_custom_target(fmt
        COMMAND ${CLANG_FORMAT} -i ${FORMAT_SOURCES}
        COMMENT "Formatting sources"
    )
endif()

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex   $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}\nBuilding ${BIN_FILE}")
