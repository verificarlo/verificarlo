#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

enum class Op { ADD, SUB, MUL, DIV, FMA, SQRT };

auto opfromstring(const char op) -> Op {
  switch (op) {
  case '+':
    return Op::ADD;
  case '-':
    return Op::SUB;
  case 'x':
    return Op::MUL;
  case '/':
    return Op::DIV;
  case 'f':
    return Op::FMA;
  case 's':
    return Op::SQRT;
  default:
    std::cerr << "Invalid operation\n";
    exit(EXIT_FAILURE);
  }
}

template <typename T>
__attribute__((noinline)) auto operation(const Op &op, T a, T b, T c) -> T {
  switch (op) {
  case Op::ADD:
    return a + b;
  case Op::SUB:
    return a - b;
  case Op::MUL:
    return a * b;
  case Op::DIV:
    return a / b;
  case Op::FMA:
    return std::fma(a, b, c);
  case Op::SQRT:
    return std::sqrt(a);
  }
}

auto main(int argc, const char *argv[]) -> int {

  if (argc < 3 || argc > 6) {
    std::cerr << "usage: ./test <float|double> <op> a [b] [c]\n";
    std::cerr << "op: +, -, x, /, s, f\n";
    return EXIT_FAILURE;
  }

  const std::string ftype = argv[1];
  const auto op = opfromstring(argv[2][0]);
  const double a = (argc > 3) ? strtod(argv[3], nullptr) : 0;
  const double b = (argc > 4) ? strtod(argv[4], nullptr) : 0;
  const double c = (argc > 5) ? strtod(argv[5], nullptr) : 0;

  if (ftype == "float") {
    const auto result = operation<float>(op, a, b, c);
    printf("%.6a\n", result);
  } else if (ftype == "double") {
    const auto result = operation<double>(op, a, b, c);
    printf("%.13a\n", result);
  } else {
    std::cerr << "Invalid type\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
