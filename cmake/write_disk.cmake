# write_disk.cmake — builds a LEGF filesystem image containing user.bin and
# writes it at sector 0 of data/disk.img.
# Called by the write_disk custom target during the build.
#
# Required variables (pass via -D):
#   BINARY   — path to user.bin
#   DISK     — path to data/disk.img
#   MKFS     — path to cmake/mkfs.py

cmake_minimum_required(VERSION 3.20)

get_filename_component(MKFS_DIR "${MKFS}" DIRECTORY)

execute_process(
    COMMAND python3 ${MKFS}
        --disk ${DISK}
        --blob init:${ELF}:exec
    WORKING_DIRECTORY ${MKFS_DIR}
    RESULT_VARIABLE res
)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "mkfs.py failed")
endif()
