// eabi.s — ARM EABI compiler runtime stubs
//
// __aeabi_memclr{,4,8}(void *dst, size_t len)  →  memclr(dst, len)
// __aeabi_memset{,4,8}(void *dst, size_t len, int c)  →  memset(dst, c, len)


.text

.global __aeabi_memclr
.global __aeabi_memclr4
.global __aeabi_memclr8
.type   __aeabi_memclr,  %function
.type   __aeabi_memclr4, %function
.type   __aeabi_memclr8, %function

__aeabi_memclr:
__aeabi_memclr4:
__aeabi_memclr8:
    b memclr

.global __aeabi_memset
.global __aeabi_memset4
.global __aeabi_memset8
.type   __aeabi_memset,  %function
.type   __aeabi_memset4, %function
.type   __aeabi_memset8, %function


__aeabi_memset:
__aeabi_memset4:
__aeabi_memset8:
    mov  r3, r1
    mov  r1, r2
    mov  r2, r3
    b    memset