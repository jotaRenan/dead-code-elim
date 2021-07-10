#include <stdio.h>

int main() {
  int i = 0;
  int sum = 0;
  for (i = 0; i < 100; i++) {
    sum += i;
    if (i > 101) {
      break;
    }
  }
  return sum;
}
