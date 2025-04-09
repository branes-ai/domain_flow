#include <iostream>
#include <string>
#include <vector>
#include <regex>

struct TensorTypeInfo {
    std::vector<int> shape;
    std::string elementType;
};

TensorTypeInfo parseTensorType(const std::string& tensorTypeStr) {
    TensorTypeInfo result;
    
    // Match the tensor type pattern: tensor<axbxcxf32>
    std::regex tensorPattern(R"(tensor<(.*?)x([^>]+)>)");
    std::smatch matches;
    
    if (std::regex_search(tensorTypeStr, matches, tensorPattern) && matches.size() >= 3) {
        std::string dimensionsStr = matches[1].str() + "x" + matches[2].str();
        
        // Split dimensions by 'x'
        size_t pos = 0;
        std::string token;
        while ((pos = dimensionsStr.find('x')) != std::string::npos) {
            token = dimensionsStr.substr(0, pos);
            
            // Check if token is a number (shape dimension) or the element type
            if (std::all_of(token.begin(), token.end(), [](char c) { return std::isdigit(c); })) {
                result.shape.push_back(std::stoi(token));
            } else {
                result.elementType = token;
                break;
            }
            
            dimensionsStr.erase(0, pos + 1);
        }
        
        // Check if the last part is the element type
        if (dimensionsStr.find_first_not_of("0123456789") != std::string::npos) {
            result.elementType = dimensionsStr;
        } else if (!dimensionsStr.empty()) {
            // Last dimension
            result.shape.push_back(std::stoi(dimensionsStr));
        }
    }
    
    return result;
}

int main() {
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