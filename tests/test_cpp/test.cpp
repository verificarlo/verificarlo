#include <cmath>
#include <iostream>
#include <string>

class A {
public:
  float x;
  double y;

public:
  A(float x, double y) : x(x), y(y){};
  A operator+(A a) { return A(this->x + a.x, this->y + a.y); };
};

int main() {
  int a = std::isinf(3.0);
  int b = std::isnan(0.0);
  std::string s = std::to_string(1);

  A x(1, 2), y(2, 3);
  A z = x + y;
  std::cout << z.x << "," << z.y << std::endl;
  return 0;
}
