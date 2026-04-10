// kbd.h — keyboard input
//   UART path:   QEMU -serial stdio forwards stdin to PL011; kbd_handle_char() called per byte
//   Virtio path: virtio-input device in QEMU graphical window

#ifndef KBD_H
#define KBD_H

#include "types.h"
#include "virtio.h"

// GIC IRQ for the virtio-input device; 0 if no device found during kbd_init().
extern u32 kbd_irq;

void kbd_handle_char(char c);
void handle_kbd_event(const virtio_input_event_t *ev);

#endif // KBD_H
