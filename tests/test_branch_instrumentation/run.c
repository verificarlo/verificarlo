#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv) {
  float taba[100];
  float tabb[100];
  int tabc[100];

  for (int i = 0; i < 100; i++) {
    taba[i] = (i % 2 == 0) ? i * 1.2f : i * 1.1f;
    tabb[i] = (i % 2 == 0) ? i * 1.1f : i * 1.2f;
  }

  for (int i = 0; i < 100; i++) {
    tabc[i] = taba[i] > tabb[i];
  }

  assert(!tabc[0]);
  assert(!tabc[1]);
  assert(tabc[2]);
  assert(tabc[98]);
}
