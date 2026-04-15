# write_disk.cmake — builds a LEGF filesystem image containing user binaries
# Called by the write_disk custom target during the build.
#
# Required variables (pass via -D):
#   ELFS     — semi-colon separated list of paths to user binaries
#   DISK     — path to data/disk.img
#   MKFS     — path to cmake/mkfs.py

cmake_minimum_required(VERSION 3.20)

# Ensure required variables are present
if (NOT DISK OR NOT MKFS)
    message(FATAL_ERROR "Usage: cmake -DELFS=\"list\" -DDISK=path -DMKFS=path -P write_disk.cmake")
endif ()

get_filename_component(MKFS_DIR "${MKFS}" DIRECTORY)

message("Packing into LEGF filesystem image at ${DISK}")

# Initialize the argument list for the python script
list(APPEND MKFS_ARGS --disk "${DISK}")

# Check if ELFS has content
if (NOT ELFS)
    message(WARNING "No ELFS provided to write_disk.cmake. Creating empty filesystem?")
else ()
    foreach (ELF IN LISTS ELFS)
        get_filename_component(ELF_NAME "${ELF}" NAME_WE)
        message(STATUS "Adding ${ELF_NAME} to filesystem image")

        # Ensure we use quotes around the argument in case of spaces in paths
        list(APPEND MKFS_ARGS --blob "${ELF_NAME}:${ELF}:exec")
    endforeach ()
endif ()

execute_process(
        COMMAND python3 "${MKFS}" ${MKFS_ARGS}
        WORKING_DIRECTORY "${MKFS_DIR}"
        RESULT_VARIABLE res
)

if (NOT res EQUAL 0)
    message(FATAL_ERROR "mkfs.py failed with return code ${res}")
endif ()