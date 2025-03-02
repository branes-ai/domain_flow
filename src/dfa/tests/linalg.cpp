#include <iostream>
#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;

    Matrix<int> A = { {1, 0}, {0, 1} };
    Matrix<int> B(A);

    Matrix<int> C = A * B;

    std::cout << C << '\n';

    return EXIT_SUCCESS;
}
