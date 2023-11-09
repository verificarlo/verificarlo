#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>

#ifndef SIZE
#error "SIZE must be defined"
#endif

#ifdef __GNUC__
using float2 = __attribute__((__vector_size__(2 * sizeof(float)))) float;
using float4 = __attribute__((__vector_size__(4 * sizeof(float)))) float;
using float8 = __attribute__((__vector_size__(8 * sizeof(float)))) float;
using float16 = __attribute__((__vector_size__(16 * sizeof(float)))) float;
using double2 = __attribute__((__vector_size__(2 * sizeof(double)))) double;
using double4 = __attribute__((__vector_size__(4 * sizeof(double)))) double;
using double8 = __attribute__((__vector_size__(8 * sizeof(double)))) double;
using double16 = __attribute__((__vector_size__(16 * sizeof(double)))) double;
using int2 = __attribute__((__vector_size__(2 * sizeof(int)))) int;
using int4 = __attribute__((__vector_size__(4 * sizeof(int)))) int;
using int8 = __attribute__((__vector_size__(8 * sizeof(int)))) int;
using int16 = __attribute__((__vector_size__(16 * sizeof(int)))) int;
#elif __clang__
typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef double double16 __attribute__((ext_vector_type(16)));
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));
typedef int int2 __attribute__((ext_vector_type(2)));
typedef int int4 __attribute__((ext_vector_type(4)));
typedef int int8 __attribute__((ext_vector_type(8)));
typedef int int16 __attribute__((ext_vector_type(16)));
#else
#error "Compiler must be gcc or clang"
#endif

template <typename T, int size> struct vectortype;

template <> struct vectortype<float, 2> {
  using type = float2;
};

template <> struct vectortype<float, 4> {
  using type = float4;
};

template <> struct vectortype<float, 8> {
  using type = float8;
};

template <> struct vectortype<float, 16> {
  using type = float16;
};

template <> struct vectortype<double, 2> {
  using type = double2;
};

template <> struct vectortype<double, 4> {
  using type = double4;
};

template <> struct vectortype<double, 8> {
  using type = double8;
};

template <> struct vectortype<double, 16> {
  using type = double16;
};

template <> struct vectortype<int, 2> {
  using type = int2;
};

template <> struct vectortype<int, 4> {
  using type = int4;
};

template <> struct vectortype<int, 8> {
  using type = int8;
};

template <> struct vectortype<int, 16> {
  using type = int16;
};

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

template <typename T, int size, typename V = typename vectortype<T, size>::type>
auto init_args(const T a) -> V {
  std::array<T, size> args{};
  for (auto &arg : args) {
    arg = a;
  }
  V result;
  std::memcpy(&result, args.data(), sizeof(V));
  return result;
}

template <typename T, int size, typename V = typename vectortype<T, size>::type>
auto fma(const V a, const V b, const V c) -> V {
  return a * b + c;
}

template <typename T, int size, typename V = typename vectortype<T, size>::type>
auto sqrt(const V a) -> V {
  V res = {0};
  for (int i = 0; i < size; i++) {
    res[i] = sqrt(a[i]);
  }
  return res;
}

template <typename T, int size, typename V = typename vectortype<T, size>::type,
          typename... Args>
auto operation(const Op &op, const Args... args) -> V {

  constexpr auto arity = sizeof...(Args);

  V a = init_args<T, size>(std::get<0>(std::make_tuple(args...)));
  V b = (arity >= 2) ? init_args<T, size>(std::get<1>(std::make_tuple(args...)))
                     : V{0};
  V c = (arity >= 3) ? init_args<T, size>(std::get<2>(std::make_tuple(args...)))
                     : V{0};

  switch (op) {
  case Op::ADD: {
    return a + b;
  case Op::SUB:
    return a - b;
  case Op::MUL:
    return a * b;
  case Op::DIV:
    return a / b;
  case Op::FMA:
    return fma<T, size>(a, b, c);
  case Op::SQRT:
    return sqrt<T, size>(a);
  default:
    std::cerr << "Invalid operation\n";
    exit(EXIT_FAILURE);
  }
  }
}

template <typename T, int size, typename V = typename vectortype<T, size>::type>
void print_vector(const V &v) {
  for (int i = 0; i < size; i++) {
    printf("%.13a ", v[i]);
  }
  printf("\n");
}

auto main(int argc, const char *argv[]) -> int {

  if (argc < 3 || argc > 6) {
    std::cerr << "usage: ./test <float|double> <op> a [b] [c]\n";
    std::cerr << "op: +, -, x, /, s, f\n";
    return EXIT_FAILURE;
  }

  constexpr int size = SIZE;

  const std::string ftype = argv[1];
  const auto op = opfromstring(argv[2][0]);
  const double a = (argc > 3) ? strtod(argv[3], nullptr) : 0;
  const double b = (argc > 4) ? strtod(argv[4], nullptr) : 0;
  const double c = (argc > 5) ? strtod(argv[5], nullptr) : 0;

  if (ftype == "float") {
    const auto result = operation<float, size>(op, a, b, c);
    print_vector<float, size>(result);
  } else if (ftype == "double") {
    const auto result = operation<double, size>(op, a, b, c);
    print_vector<double, size>(result);
  } else {
    std::cerr << "Invalid type\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
