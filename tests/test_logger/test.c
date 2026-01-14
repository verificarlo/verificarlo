#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  FILE *log_file = fopen("log", "w");
  if (log_file == NULL) {
    perror("Failed to open log file");
    return EXIT_FAILURE;
  }
  double a = 3.14;
  double b = 2.71;
  double c = a + b;
  fprintf(log_file, "%.13a+%.13a=%.13a\n", a, b, c);
  float x = 0.1f;
  float y = 0.01f;
  float z = x + y;
  fprintf(log_file, "%.6a+%.6a=%.6a\n", x, y, z);
  fclose(log_file);
  return 0;
}