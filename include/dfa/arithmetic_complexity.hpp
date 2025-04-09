#pragma once
#include <iostream>
#include <iomanip>
#include <unordered_map>

namespace sw {
    namespace dfa {

        // Define the arithmetic complexity metrics structure
        struct ArithmeticMetrics {
            // Maps: operation type -> numerical type -> count
            std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>> opMetrics;

            // Increment count for specific operation and type
            void recordOperation(const std::string& opType, const std::string& numType, uint64_t count = 1) {
                opMetrics[opType][numType] += count;
            }

            // Get total for a specific operation across all numerical types
            uint64_t getOperationTotal(const std::string& opType) const {
                uint64_t total = 0;
                auto it = opMetrics.find(opType);
                if (it != opMetrics.end()) {
                    for (const auto& [numType, count] : it->second) {
                        total += count;
                    }
                }
                return total;
            }

            // Get total for a specific numerical type across all operations
            uint64_t getNumericalTypeTotal(const std::string& numType) const {
                uint64_t total = 0;
                for (const auto& [opType, typeMap] : opMetrics) {
                    auto it = typeMap.find(numType);
                    if (it != typeMap.end()) {
                        total += it->second;
                    }
                }
                return total;
            }

            // Get all operation types being tracked
            std::vector<std::string> getOperationTypes() const {
                std::vector<std::string> types;
                for (const auto& [opType, _] : opMetrics) {
                    types.push_back(opType);
                }
                return types;
            }

            // Get all numerical types being tracked
            std::vector<std::string> getNumericalTypes() const {
                std::unordered_set<std::string> typeSet;
                for (const auto& [_, typeMap] : opMetrics) {
                    for (const auto& [numType, _] : typeMap) {
                        typeSet.insert(numType);
                    }
                }
                return std::vector<std::string>(typeSet.begin(), typeSet.end());
            }

            // Print summary report
            void printSummary() const {
                const int OPERATOR_WIDTH = 25;
                const int COL_WIDTH = 15;
                std::cout << std::setw(OPERATOR_WIDTH) << "Operator" << std::setw(COL_WIDTH) << "count" << std::setw(COL_WIDTH) << "Percentage" << std::endl;
                // gather the total
				uint64_t total = 0;
				for (const auto& opType : getOperationTypes()) {
					for (const auto& [numType, count] : opMetrics.at(opType)) {
						total += count;
					}
				}
                // By operation type
                std::cout << "=== By Operation Type ===" << std::endl;
                for (const auto& opType : getOperationTypes()) {
					auto opTotal = getOperationTotal(opType);
                    std::cout << std::setw(OPERATOR_WIDTH) << opType 
                        << std::setw(COL_WIDTH) << opTotal 
                        << std::setw(COL_WIDTH) << (opTotal * 100.0) / total 
                        << std::endl;
                    for (const auto& [numType, count] : opMetrics.at(opType)) {
                        std::cout << "  " << numType << ": " << count << std::endl;
						std::cout << std::setw(OPERATOR_WIDTH) << (std::string("   ") + numType)
                            << std::setw(COL_WIDTH) << count
                            << std::setw(COL_WIDTH) << (count * 100.0) / total
                            << std::endl;
                    }
                }

                // By numerical type
                std::cout << "\n=== By Numerical Type ===" << std::endl;
                for (const auto& numType : getNumericalTypes()) {
                    std::cout << numType << ": " << getNumericalTypeTotal(numType) << std::endl;
                    for (const auto& [opType, typeMap] : opMetrics) {
                        auto it = typeMap.find(numType);
                        if (it != typeMap.end()) {
                            std::cout << "  " << opType << ": " << it->second << std::endl;
                        }
                    }
                }
            }
        };

        // TBD: better implementation
		inline int isArithmeticTypeContained(const std::string& type1, const std::string& type2) {
			// Check if type1 is contained in type2
			if (type1 == type2) {
				return 0; // same type
			}
			else if (type1 == "f32" && (type2 == "f16" || type2 == "i8")) {
				return 1; // type2 is contained in type1
			}
			else if (type2 == "f32" && (type1 == "f16" || type1 == "i8")) {
				return 2; // type1 is contained in type2
			}
			return -1; // not contained
		}

    }
} // namespace sw::dfa