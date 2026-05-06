QEMU_OPTS=-cpu cortex-a72 -machine virt,virtualization=on -nographic -kernel hello64.elf

.PHONY: clean qemu debug run

hello64.elf: start.s link.ld
	aarch64-elf-gcc -nostdlib -ffreestanding -o hello64.elf start.s -T link.ld -g
clean:
	rm -f hello64.elf
qemu: hello64.elf
	qemu-system-aarch64 ${QEMU_OPTS}
debug: hello64.elf
	qemu-system-aarch64 ${QEMU_OPTS} -s -S
run: qemu
