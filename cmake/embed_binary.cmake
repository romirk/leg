# embed_binary.cmake — generates an assembly file that embeds INPUT as a
# read-only blob between user_image_start and user_image_end.
#
# Usage (add_custom_command -P):
#   cmake -DINPUT=<path/to/file.bin> -DOUTPUT=<path/to/out.s> -P embed_binary.cmake

file(WRITE "${OUTPUT}"
    ".section .rodata\n"
    ".global user_image_start\n"
    ".global user_image_end\n"
    ".balign 4\n"
    "user_image_start:\n"
    "\t.incbin \"${INPUT}\"\n"
    "user_image_end:\n"
)
