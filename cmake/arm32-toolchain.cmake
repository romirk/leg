set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm32)

set(TARGET_ARCH arm-none-eabi)
set(CPU cortex-a15)

set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_AR ${TARGET_ARCH}-ar)
set(CMAKE_OBJCOPY ${TARGET_ARCH}-objcopy)
set(CMAKE_OBJDUMP ${TARGET_ARCH}-objdump)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

add_compile_options(-mfloat-abi=hard -mfpu=neon-vfpv4)
add_compile_options(-mcpu=${CPU} -Wall -Wextra --target=${TARGET_ARCH} -ffreestanding -fbuiltin -flto)
add_link_options(-mfloat-abi=hard -mfpu=neon-vfpv4 -flto -fuse-ld=lld --target=${TARGET_ARCH} -mcpu=${CPU} -nostdlib)
