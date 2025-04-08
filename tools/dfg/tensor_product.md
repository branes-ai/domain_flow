# Tensor Product demonstration

```cpp
#include <vector>
#include <iostream>
#include <stdexcept>

// Template function for tensor matmul
template <typename T>
std::vector<T> tensorMatmul(const std::vector<T>& arg0,
                             const std::vector<T>& arg1,
                             const std::vector<int>& shape0,
                             const std::vector<int>& shape1,
                             std::vector<int>& resultShape) {

    if (shape0.size() != 3 || shape1.size() != 3) {
        throw std::invalid_argument("Input tensors must have 3 dimensions.");
    }

    int a = shape0[0], b = shape0[1], c = shape0[2];
    int d = shape1[0], e = shape1[1], f = shape1[2];

    if (c != e) {
        throw std::invalid_argument("Inner dimensions must match.");
    }

    resultShape = {a, b, f};
    std::vector<T> result(a * b * f, 0);

    for (int i = 0; i < a; ++i) {
        for (int j = 0; j < b; ++j) {
            for (int ff = 0; ff < f; ++ff) {
                for(int dd = 0; dd < d; ++dd){
                    T sum = 0;
                    for (int k = 0; k < c; ++k) {
                        sum += arg0[i * b * c + j * c + k] * arg1[dd * e * f + k * f + ff];
                    }
                    result[i * b * f + j * f + ff] = sum;
                }
            }
        }
    }

    return result;
}

int main() {
    std::vector<float> arg0 = {
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
        8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f,
        15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f,
        22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f,
        28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f,
    };
    std::vector<float> arg1 = {
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f,
        10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f,
        19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f,
        28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f,
        37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f,
        46.0f, 47.0f, 48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f,
    };

    std::vector<int> shape0 = {3, 5, 7};
    std::vector<int> shape1 = {2, 7, 9};
    std::vector<int> resultShape;

    std::vector<float> result = tensorMatmul(arg0, arg1, shape0, shape1, resultShape);

    std::cout << "Result Shape: ";
    for (int dim : resultShape) {
        std::cout << dim << " ";
    }
    std::cout << std::endl;

    std::cout << "Result: ";
    for (float val : result) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**Explanation:**

1.  **Template Function:**
    * The `tensorMatmul` function is a template, allowing it to work with various data types (e.g., `float`, `double`, `int`).
    * It takes the input tensors (`arg0`, `arg1`), their shapes (`shape0`, `shape1`), and a `resultShape` vector to store the output shape.

2.  **Dimension Validation:**
    * It checks if the input tensors have 3 dimensions and if the inner dimensions match (`c == e`).

3.  **Result Shape Calculation:**
    * The output shape is calculated as `{a, b, f}`.

4.  **Matrix Multiplication:**
    * The nested loops iterate through the output tensor's dimensions (`i`, `j`, `ff`).
    * An inner loop iterates through the shared dimension (`k`) to perform the summation.
    * The result is stored in the `result` vector.
    * The d dimension is handled by adding another loop that iterates through the d dimension, and performs the calculation for each iteration.

5.  **Main Function:**
    * Example input tensors and shapes are created.
    * The `tensorMatmul` function is called.
    * The output shape and result are printed.

**Important Notes:**

* This code assumes that the input tensors are stored in a flattened 1D `std::vector`.
* The indexing calculations (`i * b * c + j * c + k`) are used to access the correct elements in the flattened vectors.
* This code is a basic implementation and can be optimized for performance.
* This code will throw an exception if the input tensors are not valid.
* This code performs the matrix multiplcation for each batch of the second input tensor, and stores the results into the output tensor.
