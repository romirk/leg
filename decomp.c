#include <stdint.h>

uint64_t x[32], *sp;
uint32_t *w = (uint32_t *)x;

void f(uint32_t a, uint32_t b, uint32_t p, uint32_t q, uint32_t r,
       uint32_t ret) {
  w[0] = a;
  w[1] = b;
  w[2] = p;
  w[3] = q;
  w[4] = r;
  w[5] = ret;

  sp -= 8;

  sp[0] = x[22];
  sp[1] = x[21];

  int cmp = w[2] < 1;

  sp[2] = x[20];
  sp[3] = x[19];

  if (cmp)
    goto b11c;

  cmp = w[4] < 1;

  if (cmp)
    goto b11c;

  cmp = w[3] <= 0;

  w[8] = w[4];
  w[9] = w[2];

  if (cmp)
    goto b128;

  cmp = w[3] > 7;

  w[11] = w[3];
  x[12] = x[9] << 2;
  cmp = cmp ? w[4] == 1 : 0;
  x[13] = x[8] << 2;
  x[10] = 0;
  w[14] = cmp;
  x[15] = x[11] & 0x7ffffff8;
  x[16] = x[0] + 16;
  x[17] = x[1] + 16;
  goto b68;

b54:
  x[10]++;
  x[16] += x[12];
  x[0] += x[12];

  cmp = x[10] == x[9];
  if (cmp)
    goto b11c;

b68:
  x[2] = x[10] * x[8];
  x[18] = 0;
  x[3] = x[1];
  x[4] = x[17];
  x[2] = x[5] + (x[2] << 2);
  goto b98;

b98:
b11c:
b128:;
}
