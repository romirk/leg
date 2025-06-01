#!/bin/zsh

arm-none-eabi-objdump  --disassembler-color=extended --visualize-jumps=color -Dx cmake-build-debug-arm32/leg.elf | less -R
