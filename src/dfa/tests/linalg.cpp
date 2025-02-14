#include <iostream>


int main() {
	    DenseMatrix m{{1, 2, 3}, {4, 5, 6}};
    std::cout << m[0][1] << std::endl;  // Output: 2
    m[1][2] = 9;  // Modify an element
    std::cout << m[1][2] << std::endl;  // Output: 9

    DenseVector v{7, 8, 9};
    std::cout << v[1] << std::endl;  // Output: 8
    v[2] = 10;  // Modify an element
    std::cout << v[2] << std::endl;  // Output: 10
				     
    AffineMap map{{ {2, 1}, {1, 3} }, {3, 4}};
    DenseVector v{1, 2};
    DenseVector result = map.apply(v);

    for (size_t i = 0; i < result.size(); ++i) {
        std::cout << result.get(i) << " ";
    }
    std::cout << std::endl; // Output: 7 11

      AffineMap map2{{ {2, 1, 4}, {1, 3, 5}, {4, 5, 2} }, {3, 4, 1}};
    DenseVector v2{1, 2, 3};
    DenseVector result2 = map2.apply(v2);

    for (size_t i = 0; i < result2.size(); ++i) {
        std::cout << result2.get(i) << " ";
    }
    std::cout << std::endl; // Output: 21 24 22


    return 0;
}
