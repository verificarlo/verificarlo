#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../interflop_stdlib.c"

#define STR_LIM 2048

/* generate a random string of size <size> */
char *generate_string(size_t size) {
  char *str = (char *)malloc(sizeof(char) * (size + 1));
  for (int i = 0; i < size; i++) {
    str[i] = rand() % 254 + 1;
  }
  str[size] = '\0';
  return str;
}

/* __string_equal returns 1 if strings are equal and 0 otherwise */
/* strcmp returns 0 if string are equal and 1 or -1 otherwise */
void assert_equal(const char *s1, const char *s2) {
  int a = __string_equal(s1, s2);
  int b = strcmp(s1, s2);

  if (b != 0) {
    fprintf(stderr, "Error: strcmp(%s,%s) is false\n", s1, s2);
    assert(0);
  }

  if (a == 0) {
    fprintf(stderr, "Error: __string_equal(%s,%s) is false\n", s1, s2);
    assert(0);
  }
}

void assert_not_equal(const char *s1, const char *s2) {
  int a = __string_equal(s1, s2);
  int b = strcmp(s1, s2);

  if (b == 0) {
    fprintf(stderr, "Error: strcmp(%s,%s) is true\n", s1, s2);
    assert(0);
  }

  if (a == 1) {
    fprintf(stderr, "Error: __string_equal(%s,%s) is true\n", s1, s2);
    assert(0);
  }
}

int main(int argc, char *argv[]) {

  char *strings[STR_LIM];
  for (int i = 1; i < STR_LIM; i++) {
    strings[i] = generate_string(i);
  }

  for (int i = 1; i < STR_LIM; i++) {
    for (int j = 1; j < STR_LIM; j++) {
      const char *s1 = strings[i];
      const char *s2 = strings[j];

      assert_equal(s1, s1);
      assert_equal(s2, s2);
      if (i != j) {
        assert_not_equal(s1, s2);
        assert_not_equal(s2, s1);
      } else {
        assert_equal(s1, s2);
        assert_equal(s2, s1);
      }
    }
  }

  fprintf(stderr, "Test passed\n");
}