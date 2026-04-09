cmake_minimum_required(VERSION 3.30)

set(MCPU cortex-a15)

# project settings
project(leg C ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker.ld)

add_compile_options(-mfloat-abi=hard -mfpu=vfpv4)
add_link_options(-mfloat-abi=hard -mfpu=vfpv4)

#add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)
add_compile_options(-mcpu=${MCPU} -Wall -Wextra --target=${TARGET_ARCH} -ffreestanding -fbuiltin -flto)


add_link_options(-flto -fuse-ld=lld --target=${TARGET_ARCH} -mcpu=${MCPU} -Wl,-T,${LINKER_SCRIPT} -nostdlib)

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

include_directories(
        ${CMAKE_SOURCE_DIR}/include
)

#add_definitions(#[[TODO insert definitions, ex. -DUSE_HAL_DRIVER -DCORE_CM4 -DDEBUG -DSTM32H745xx]])

file(GLOB_RECURSE SOURCES
        "src/*.*"
)

#add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
#set(CMAKE_EXE_LINKER_FLAGS "--target=${TARGET_ARCH} -flto -mcpu=${MCPU} -Wl,-T,\"${LINKER_SCRIPT}\" -nostdlib")

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

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

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
