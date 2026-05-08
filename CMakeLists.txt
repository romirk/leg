cmake_minimum_required(VERSION 4.0)

# project settings
project(leg C ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/link/linker_${CMAKE_SYSTEM_PROCESSOR}.ld)
set(USER_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/src/usr/linker_user.ld)

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
    add_compile_options(-Og)
    add_compile_definitions(LOG_DEBUG)
endif ()

include_directories(${CMAKE_SOURCE_DIR}/include)

# ── User binary ───────────────────────────────────────────────────────────────

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm32")
    add_subdirectory(src/usr)
    get_property(USR_TARGETS DIRECTORY src/usr PROPERTY BUILDSYSTEM_TARGETS)
    foreach (TARGET IN LISTS USR_TARGETS)
        list(APPEND USR_ELFS ${PROJECT_BINARY_DIR}/src/usr/${TARGET})
    endforeach ()
    message(STATUS "User binaries: ${USR_ELFS}")

    set(DISK_IMG ${CMAKE_SOURCE_DIR}/data/disk.img)

    add_custom_target(write_disk
            COMMAND ${CMAKE_COMMAND}
            "-DELFS=${USR_ELFS}"  # Use quotes to preserve the list format
            "-DDISK=${DISK_IMG}"
            "-DMKFS=${CMAKE_SOURCE_DIR}/legfs/mkfs.py"
            -P "${CMAKE_SOURCE_DIR}/cmake/write_disk.cmake"
            DEPENDS ${USR_ELFS}
            COMMENT "Packing user binaries into LEGF filesystem image"
            VERBATIM
    )
endif ()

# ── Kernel binary ─────────────────────────────────────────────────────────────

file(GLOB_RECURSE KERNEL_SOURCES "src/kernel/*.*")
list(FILTER KERNEL_SOURCES EXCLUDE REGEX ".*/arch/.*")

file(GLOB_RECURSE ARCH_SOURCES "src/kernel/arch/${CMAKE_SYSTEM_PROCESSOR}/*")
file(GLOB_RECURSE LIBC_SOURCES "src/libc/*.*")

add_executable(${PROJECT_NAME}.elf
#        ${KERNEL_SOURCES}
        ${ARCH_SOURCES}
#        ${LIBC_SOURCES}
        ${LINKER_SCRIPT}
)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm32")
    target_sources(${PROJECT_NAME}.elf PRIVATE ${KERNEL_SOURCES} ${LIBC_SOURCES})
    add_dependencies(${PROJECT_NAME}.elf write_disk)
endif ()

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE KERNEL)
target_link_options(${PROJECT_NAME}.elf PRIVATE -Wl,-T,${LINKER_SCRIPT})

find_program(CLANG_FORMAT clang-format)
if (CLANG_FORMAT)
    file(GLOB_RECURSE FORMAT_SOURCES
            "src/*.c" "src/*.h"
            "include/*.h"
    )
    list(FILTER FORMAT_SOURCES EXCLUDE REGEX ".*/fonts/.*")
    add_custom_target(fmt
            COMMAND ${CLANG_FORMAT} -i ${FORMAT_SOURCES}
            COMMENT "Formatting sources"
    )
endif ()

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}\nBuilding ${BIN_FILE}")
