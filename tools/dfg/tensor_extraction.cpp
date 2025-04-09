#include <iostream>
#include <string>
#include <vector>
#include <dfa/tensor_spec_parser.hpp>

int main() {
	using namespace sw::dfa;

    std::vector<std::string> tests = {
        "tensor<1xf8>",
        "tensor<1x2xf8>",
        "tensor<1x2x3xf8>",
        "tensor<1x2x3x4xf8>",
		"tensor<1xf16>",
		"tensor<1x2xf16>",
		"tensor<1x2x3xf16>",
		"tensor<1x2x3x4xf16>",
        "tensor<1xf32>",
        "tensor<1x2xf32>",
        "tensor<1x2x3xf32>",
        "tensor<1x2x3x4xf32>",

        "tensor<1xi8>",
        "tensor<1x2xi8>",
        "tensor<1x2x3xi8>",
        "tensor<1x2x3x4xi8>",
        "tensor<1xi16>",
        "tensor<1x2xi16>",
        "tensor<1x2x3xi16>",
        "tensor<1x2x3x4xi16>",
        "tensor<1xi32>",
        "tensor<1x2xi32>",
        "tensor<1x2x3xi32>",
        "tensor<1x2x3x4xi32>",

        "tensor<1xsi8>",
        "tensor<1x2xsi8>",
        "tensor<1x2x3xsi8>",
		"tensor<1x2x3x4xsi8>",
        "tensor<1x2x3x4xsi16>",
        "tensor<1x2x3x4xsi32>",
        "tensor<1xui8>",
        "tensor<1x2xui8>",
        "tensor<1x2x3xui8>",
        "tensor<1x2x3x4xui8>",
        "tensor<1x2x3x4xui16>",
        "tensor<1x2x3x4xui32>",

        // an undefined batch dimension
        "tensor<?x2x3x4xf8>",
        "tensor<?x2x3x4xf16>",
        "tensor<?x2x3x4xf32>",
        "tensor<?x2x3x4xf64>",
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