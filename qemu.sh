#!/bin/sh
qemu-system-arm -machine virt -gdb tcp::17735 -S -nographic -m 2G -kernel cmake-build-debug-arm32/leg.elf
