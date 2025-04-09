#pragma once

#include <string>
#include <vector>
#include <regex>

namespace sw {
    namespace dfa {

        struct TensorTypeInfo {
            std::vector<int> shape;
            std::string elementType;
        };

        TensorTypeInfo parseTensorType(const std::string& tensorTypeStr) {
            TensorTypeInfo result;

			std::string workingStr = tensorTypeStr;
            // convert an undefined batch dimension to 1
            if (workingStr.find_first_of('?') != std::string::npos) {
                workingStr.replace(tensorTypeStr.find_first_of('?'), 1, "1");
			}
            // Match the tensor type pattern: tensor<axbxcxf32>
            std::regex tensorPattern(R"(tensor<(.*?)x([^>]+)>)");
            std::smatch matches;

            if (std::regex_search(workingStr, matches, tensorPattern) && matches.size() >= 3) {
                std::string dimensionsStr = matches[1].str() + "x" + matches[2].str();

                // Split dimensions by 'x'
                size_t pos = 0;
                std::string token;
                while ((pos = dimensionsStr.find('x')) != std::string::npos) {
                    token = dimensionsStr.substr(0, pos);

                    // Check if token is a number (shape dimension) or the element type
                    if (std::all_of(token.begin(), token.end(), [](char c) { return std::isdigit(c); })) {
                        result.shape.push_back(std::stoi(token));
                    }
                    else {
                        result.elementType = token;
                        break;
                    }

                    dimensionsStr.erase(0, pos + 1);
                }

                // Check if the last part is the element type
                if (dimensionsStr.find_first_not_of("0123456789") != std::string::npos) {
                    result.elementType = dimensionsStr;
                }
                else if (!dimensionsStr.empty()) {
                    // Last dimension
                    result.shape.push_back(std::stoi(dimensionsStr));
                }
            }

            return result;
        }

    }
} // namespace sw::dfa
