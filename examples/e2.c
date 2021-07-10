#include <stdio.h>

int foo(int k) {
  int i = 0;
  if (k % 2) {
    i++;
  }
  return i & 1;
}

int main() {
  int a = foo(9);
  int b = foo(6);
  printf("%d", a);
  printf("%d", b);
}

