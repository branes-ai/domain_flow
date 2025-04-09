#include <iostream>
#include <string>
#include <vector>
#include <dfa/tensor_spec_parser.hpp>

int main() {
	using namespace sw::dfa;

    std::vector<std::string> tests = {
		"tensor<1xf16>",
		"tensor<1x2xf16>",
		"tensor<1x2x3xf16>",
		"tensor<1x2x3x4xf16>",
        "tensor<1xf32>",
        "tensor<1x2xf32>",
        "tensor<1x2x3xf32>",
        "tensor<1x2x3x4xf32>",
        "tensor<2x3x4xf8>"
    };

    for (auto& test : tests) {
        TensorTypeInfo info = parseTensorType(test);

        std::cout << "Tensor shape: [";
        for (size_t i = 0; i < info.shape.size(); ++i) {
            std::cout << info.shape[i];
            if (i < info.shape.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "] Element type: " << info.elementType << std::endl;
    }
    
    return 0;
}