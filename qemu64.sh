#!/bin/sh
# AArch64 boot. Pass `el2` as the first arg to enter at EL2 instead of EL1.
MACHINE="virt"
[ "$1" = "el2" ] && MACHINE="virt,virtualization=on"

qemu-system-aarch64 \
    -machine "$MACHINE" \
    -cpu cortex-a72 \
    -m 1G \
    -device loader,file=cmake-build-debug-aarch64/leg.elf,cpu-num=0 \
    -serial stdio \
    -monitor none \
    -nographic