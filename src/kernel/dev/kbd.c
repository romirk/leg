#include "kernel/dev/kbd.h"
#include "kernel/dev/virtio.h"
#include "kernel/tty.h"

#define EV_KEY 1u
#define KEY_Q  16u

#define KEY_LEFTSHIFT  42u
#define KEY_RIGHTSHIFT 54u

u32         kbd_irq;
static bool g_shift;

// Linux keycode → ASCII, unshifted and shifted.
// Index = Linux keycode (0–57 covers all basic keys).
constexpr char keymap_lower[58] = {
    0,   0,                      // 0 reserved, 1 ESC
    '1', '2',  '3',  '4',  '5',  // 2-6
    '6', '7',  '8',  '9',  '0',  // 7-11
    '-', '=',  '\b', '\t',       // 12-15
    'q', 'w',  'e',  'r',  't',  // 16-20
    'y', 'u',  'i',  'o',  'p',  // 21-25
    '[', ']',  '\n', 0,          // 26-29 (29 = LEFTCTRL)
    'a', 's',  'd',  'f',  'g',  // 30-34
    'h', 'j',  'k',  'l',        // 35-38
    ';', '\'', '`',  0,    '\\', // 39-43 (42 = LEFTSHIFT)
    'z', 'x',  'c',  'v',  'b',  // 44-48
    'n', 'm',  ',',  '.',  '/',  // 49-53
    0,   '*',  0,    ' ',        // 54-57 (54=RIGHTSHIFT, 56=LEFTALT)
};

constexpr char keymap_upper[58] = {
    0,    0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',  'Z',
    'X',  'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ',
};

void handle_kbd_event(const virtio_input_event_t *ev) {
    if (ev->type == EV_KEY) {
        if (ev->code == KEY_LEFTSHIFT || ev->code == KEY_RIGHTSHIFT)
            g_shift = ev->value != 0;
        else if ((ev->value == 1 || ev->value == 2) && ev->code < 58u) {
            const char c = g_shift ? keymap_upper[ev->code] : keymap_lower[ev->code];
            if (c) tty_input(c);
        }
    }
}

void kbd_handle_char(const char c) {
    tty_input(c);
}
