# write_disk.cmake — writes the user binary into the disk image at sector 0.
# Called by the write_disk custom target during the build.
#
# Required variables (pass via -D):
#   BINARY  — path to user.bin
#   DISK    — path to data/disk.img

cmake_minimum_required(VERSION 3.20)

get_filename_component(DISK_DIR "${DISK}" DIRECTORY)
file(MAKE_DIRECTORY "${DISK_DIR}")

if(NOT EXISTS "${DISK}")
    message(STATUS "Creating blank disk image: ${DISK}")
    execute_process(
        COMMAND dd if=/dev/zero bs=1048576 count=64 of=${DISK}
        RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "Failed to create disk image")
    endif()
endif()

execute_process(
    COMMAND dd if=${BINARY} of=${DISK} bs=512 seek=0 conv=notrunc,sync
    RESULT_VARIABLE res
)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "Failed to write user binary to disk image")
endif()