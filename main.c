void matmul(int *a, int *b, int p, int q, int r, int *ret) {
  for (int i = 0; i < p; i++) {
    for (int j = 0; j < r; j++) {
      int sum = 0;
      for (int k = 0; k < q; k++)
        sum += a[i * p + k] * b[k * r + j];
      ret[i * r + j] = sum;
    }
  }
}
