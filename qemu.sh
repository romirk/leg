#!/bin/sh
[ -f data/disk.img ] || qemu-img create -f raw data/disk.img 64M

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