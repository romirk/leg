set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TARGET_ARCH aarch64-elf)
set(CPU cortex-a72)

set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_AR ${TARGET_ARCH}-ar)
set(CMAKE_OBJCOPY ${TARGET_ARCH}-objcopy)
set(CMAKE_OBJDUMP ${TARGET_ARCH}-objdump)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

add_compile_options(-mcpu=${CPU} --target=aarch64-elf -ffreestanding -fbuiltin -flto -Wall -Wextra)
add_link_options(-flto -fuse-ld=lld --target=${TARGET_ARCH} -mcpu=${CPU} -nostdlib)
