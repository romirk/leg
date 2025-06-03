cmake_minimum_required(VERSION 3.17)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

# specify cross compilers and tools
set(MCPU cortex-a15)
set(TARGET_ARCH arm-none-eabi)

set(CMAKE_C_COMPILER clang)
#set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_AR ${TARGET_ARCH}-ar)
set(CMAKE_OBJCOPY ${TARGET_ARCH}-objcopy)
set(CMAKE_OBJDUMP ${TARGET_ARCH}-objdump)
set(CMAKE_CUSTOM_LINKER ld64)
set(SIZE ${TARGET_ARCH}-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(leg C ASM)
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

#Uncomment for hardware floating point
#add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
#add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
#add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)

#Uncomment for software floating point
add_compile_options(-mfloat-abi=soft)

add_compile_options(-mcpu=${MCPU})
#add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)
add_compile_options(-Wall -Wextra -Werror --target=${TARGET_ARCH} -ffreestanding)

# TODO: REMOVE THIS!
add_compile_options(-Wno-arm-interrupt-vfp-clobber)

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Wno-unused-command-line-argument")


# uncomment to mitigate c++17 absolute addresses warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-O3)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-O3 -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-O1 -fno-slp-vectorize -g)
endif ()

if (DEFINED ENV{LOG_LEVEL})
    add_compile_definitions(LOG_$ENV{LOG_LEVEL})
endif ()

include_directories(
        ${CMAKE_SOURCE_DIR}/include
)

#add_definitions(#[[TODO insert definitions, ex. -DUSE_HAL_DRIVER -DCORE_CM4 -DDEBUG -DSTM32H745xx]])

file(GLOB_RECURSE SOURCES
        "src/*.*"
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker.ld)

#add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
#add_link_options(-mcpu=${MCPU} -mthumb)
add_link_options(-flto --target=${TARGET_ARCH} -mcpu=${MCPU} -Wl,-T,${LINKER_SCRIPT} -nostdlib)
#set(CMAKE_EXE_LINKER_FLAGS "--target=${TARGET_ARCH} -flto -mcpu=${MCPU} -Wl,-T,\"${LINKER_SCRIPT}\" -nostdlib")

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
