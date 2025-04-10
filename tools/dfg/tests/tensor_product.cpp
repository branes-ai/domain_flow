#include <vector>
#include <iostream>
#include <iomanip>
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

void print_tensor(const std::vector<float>& tensor, const std::vector<int>& shape) {
    for (int i = 0; i < shape[0]; ++i) {
        std::cout << "\nSlice " << i << ":\n";
        for (int j = 0; j < shape[1]; ++j) {
            for (int k = 0; k < shape[2]; ++k) {
                std::cout << std::setw(6) << tensor[i * shape[1] * shape[2] + j * shape[2] + k] << " ";
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    }
}

int main() {
	constexpr int a = 3, b = 5, c = 7;
	constexpr int d = 2, e = 7, f = 9;
	std::vector<float> arg0(a * b * c, 0.0f);  // 3 * 5 * 7
	std::vector<float> arg1(d * e * f, 0.0f);  // 2 * 7 * 9

	for (int slice = 0; slice < a; ++slice) {
		for (int row = 0; row < b; ++row) {
			for (int col = 0; col < c; ++col) {
				arg0[slice * b * c + row * c + col] = static_cast<float>(slice * b * c + row * c + col);
			}
		}
	}
	print_tensor(arg0, { a, b, c });
	for (int slice = 0; slice < d; ++slice) {
		for (int row = 0; row < e; ++row) {
			for (int col = 0; col < f; ++col) {
				arg1[slice * e * f + row * f + col] = static_cast<float>(slice * e * f + row * f + col);
			}
		}
	}
	print_tensor(arg1, { d, e, f });

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
	print_tensor(result, resultShape);

    return 0;
}