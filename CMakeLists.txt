cmake_minimum_required(VERSION 3.30)

set(MCPU cortex-a15)

# project settings
project(leg C ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(LINKER_SCRIPT      ${CMAKE_SOURCE_DIR}/linker.ld)
set(USER_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker_user.ld)

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

option(NO_ANSI "Disable ANSI escape codes in output" OFF)
if(NO_ANSI)
    add_compile_definitions(NO_ANSI)
endif ()

include_directories(${CMAKE_SOURCE_DIR}/include)

# ── User binary ───────────────────────────────────────────────────────────────

add_subdirectory(src/usr)

set(USER_BIN     ${CMAKE_CURRENT_BINARY_DIR}/src/usr/user.bin)
set(USER_IMAGE_S ${CMAKE_CURRENT_BINARY_DIR}/src/usr/user_image.s)

add_custom_command(
    OUTPUT  ${USER_BIN}
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:user.elf> ${USER_BIN}
    DEPENDS user.elf
    COMMENT "Extracting user.bin"
)

add_custom_command(
    OUTPUT  ${USER_IMAGE_S}
    COMMAND ${CMAKE_COMMAND}
            -DINPUT=${USER_BIN}
            -DOUTPUT=${USER_IMAGE_S}
            -P ${CMAKE_SOURCE_DIR}/cmake/embed_binary.cmake
    DEPENDS ${USER_BIN}
    COMMENT "Generating user_image.s"
)

# ── Kernel binary ─────────────────────────────────────────────────────────────

file(GLOB_RECURSE KERNEL_SOURCES "src/kernel/*.*")
file(GLOB_RECURSE LIBC_SOURCES   "src/libc/*.c")

add_executable(${PROJECT_NAME}.elf
    ${KERNEL_SOURCES}
    ${LIBC_SOURCES}
    ${USER_IMAGE_S}
    ${LINKER_SCRIPT}
)
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
