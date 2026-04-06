#!/bin/sh
qemu-system-arm -machine virt -m 1G -device loader,file=cmake-build-debug-arm32/leg.elf -device ramfb -serial stdio -monitor none
