#include <iostream>
int main() {
    std::cout << "C++ Test Program" << std::endl;
    #ifdef PACKAGE_STRING
    std::cout << "PACKAGE_STRING: " << PACKAGE_STRING << std::endl;
    #endif
    #ifdef TEST_MACRO
    std::cout << "TEST_MACRO: " << TEST_MACRO << std::endl;
    #endif
    return 0;
}
