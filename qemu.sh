#!/bin/sh
# data/disk.img is written by the build (cmake write_disk target).
# The fallback here handles running qemu without a prior build.
[ -f data/disk.img ] || { echo "error: data/disk.img not found — run cmake build first"; exit 1; }

qemu-system-arm \
    -machine virt \
    -m 1G \
    -device loader,file=cmake-build-debug-arm32/leg.elf \
    -serial stdio \
    -monitor none \
    -device ramfb \
    -device virtio-keyboard-device \
    -drive  file=data/disk.img,format=raw,if=none,id=blk0 \
    -device virtio-blk-device,drive=blk0