#!/bin/sh
qemu-system-arm  -machine virt -gdb tcp::17735 -nographic -kernel cmake-build-debug-arm32/leg.elf -m 2G
