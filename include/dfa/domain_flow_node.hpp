#pragma once

// extendable graph data structure
#include <graph/graph.hpp>
#include <dfa/domain_flow_operator.hpp>
#include <dfa/tensor_spec_parser.hpp>
#include <dfa/arithmetic_complexity.hpp>

namespace sw {
    namespace dfa {

        // the Domain Flow Graph node type
        struct DomainFlowNode {
            DomainFlowOperator opType;               // domain flow operator type
            std::string name;                        // source dialect name
            std::vector<std::string> operandType;    // string version of mlir::Type
            std::vector<std::string> resultValue;    // string version of mlir::Value: typically too verbose
            std::vector<std::string> resultType;     // string version of mlir::Type
            int depth;                               // depth of 0 represents a data source

            // Constructor to initialize the node with just a string of the operator
            DomainFlowNode() : opType{ DomainFlowOperator::UNKNOWN }, name{ "undefined" }, operandType{}, resultValue{}, resultType{}, depth { 0 } {}
            DomainFlowNode(const std::string& name) : opType{ DomainFlowOperator::UNKNOWN }, name{ name }, operandType{}, resultValue{}, resultType{}, depth{ 0 } {}
            DomainFlowNode(DomainFlowOperator opType, const std::string& name) : opType{ opType }, name{ name }, operandType{}, resultValue{}, resultType{}, depth{ 0 } {}

            // Modifiers
            void setOperator(DomainFlowOperator opType, std::string name) { this->opType = opType;  this->name = name; }
            void setDepth(int d) { depth = d; }

            DomainFlowNode& addOperand(const std::string& typeStr) {
                operandType.push_back(typeStr);
                return *this;
            }
            DomainFlowNode& addResult(const std::string& valueStr, const std::string& typeStr) {
                resultValue.push_back(valueStr);
                resultType.push_back(typeStr);
                return *this;
            }

            // selectors
			DomainFlowOperator getOperator() const noexcept { return opType; }
            std::string getName() const noexcept { return name; }
            int getDepth() const noexcept { return depth; }
			std::size_t getNrInputs() const noexcept { return operandType.size(); }
			std::size_t getNrOutputs() const noexcept { return resultType.size(); }
            std::string getResultValue(std::size_t idx) const { if (idx < resultValue.size()) return resultValue[idx]; else return "out of bounds"; }
            std::string getResultType(std::size_t idx) const noexcept { if (idx < resultType.size()) return resultType[idx]; else return "out of bounds"; }
        
            // Functional operators
            std::vector<std::tuple<std::string, std::string, std::uint64_t>> getArithmeticComplexity() const noexcept {
                std::vector<std::tuple<std::string, std::string, std::uint64_t>> work;
                std::tuple<std::string, std::string, std::uint64_t> stats{};
                std::stringstream ss;
				switch (opType) {
				case DomainFlowOperator::ADD:
                case DomainFlowOperator::SUB:
                    {
                        // element-wise operators, two operands
                        // Elementwise addition.
                        //    %out = tosa.add %in1, %in2 : tensor<12x6xf32>, tensor<12x6xf32>->tensor<12x6xf32>
                        // Elementwise addition with broadcasting.
                        //    %out = tosa.add %in1, %in2 : tensor<12x6xsi32>, tensor<1x1xsi32>->tensor<12x6xsi32>
                        auto tensorInfo = parseTensorType(operandType[0]);
                        std::uint64_t count{ 1 };
                        for (auto& dim : tensorInfo.shape) {
                            count *= dim;
                        }
                        stats = { "Add/Sub", tensorInfo.elementType, count };
						work.push_back(stats);
                    }
                    break;
                case DomainFlowOperator::MUL:
                    {
                        // element-wise operators, two operands
                        // Elementwise multiplication.
                        //    %out = tosa.mull %in1, %in2 : tensor<12x6xf32>, tensor<12x6xf32>->tensor<12x6xf32>
                        // Elementwise multiplication with broadcasting.
                        //    %out = tosa.mull %in1, %in2 : tensor<12x6xsi32>, tensor<1x1xsi32>->tensor<12x6xsi32>
                        auto tensorInfo = parseTensorType(operandType[0]);
                        std::uint64_t count{ 1 };
                        for (auto& dim : tensorInfo.shape) {
                            count *= dim;
                        }
                        stats = { "Multiply", tensorInfo.elementType, count };
						work.push_back(stats);
                    }
					break;
                case DomainFlowOperator::MATMUL:
                    {
                        auto tensor1 = parseTensorType(operandType[0]);
                        auto tensor2 = parseTensorType(operandType[1]);
                        std::uint16_t count{ 0 };
                        int a = tensor1.shape[0];
                        int b = tensor1.shape[1];
                        int c = tensor1.shape[2];
                        int d = tensor2.shape[0];
                        int e = tensor2.shape[1];
                        int f = tensor2.shape[2];
                        if (tensor1.elementType != tensor2.elementType) {
                            int sizeOf1 = a * b * c;
							int sizeOf2 = d * e * f;
                            // instpect types to see which tensor needs to be converted by inspecting the type conversion rules
							int typeConversion = isArithmeticTypeContained(tensor1.elementType, tensor2.elementType); // 0 = same type, 1 = contained, 2 = not contained
							switch (typeConversion) { 
                            case 1:
                                // type 2 is contained in type 1 so we need to convert type 2 to type 1
                                ss.clear();
                                ss << "Convert_" << tensor2.elementType << "_to_" << tensor1.elementType;
                                stats = { ss.str(), tensor2.elementType, sizeOf2 };
                                break;
                            case 2:
                                // type 2 is NOT contained in type 1 so we need to convert type 1 to type 2
                                ss.clear();
                                ss << "Convert_" << tensor1.elementType << "_to_" << tensor2.elementType;
                                stats = { ss.str(), tensor1.elementType, sizeOf1 };
                                break;
                            default:
								// same type, no conversion needed
								break;
							}
							// add the conversion to the stats
							work.push_back(stats);
						}
						// check if the tensors are compatible for matrix multiplication    
                        if (c == e) {
                            count = d * a * b * f;
                            stats = { "Fused Multiply", tensor1.elementType, count };
							work.push_back(stats);
							stats = { "Add", tensor1.elementType, count };
							work.push_back(stats);
						}
                        else {
                            std::cerr << "Error: incompatible tensor dimensions for matrix multiplication" << std::endl;
                        }
                    }
					break;
				case DomainFlowOperator::CONV2D:
					
					break;
				default:
                    break;
				}
                return work;
            }
        };

		bool operator==(const DomainFlowNode& lhs, const DomainFlowNode& rhs) {
			return (lhs.opType == rhs.opType) && (lhs.name == rhs.name) && (lhs.operandType == rhs.operandType)
				&& (lhs.resultValue == rhs.resultValue) && (lhs.resultType == rhs.resultType) && (lhs.depth == rhs.depth);
		}
        bool operator!=(const DomainFlowNode& lhs, const DomainFlowNode& rhs) {
            return !(lhs == rhs);
        }

        // Output stream operator
        std::ostream& operator<<(std::ostream& os, const DomainFlowNode& node) {
            // Format: name|operator|depth|operandType1,operandType2|resultValue1,resultValue2|resultType1,resultType2
            os << node.name << "|";
            os << node.opType << "|";
            os << node.depth << "|";

            // operandType
            bool first = true;
            for (const auto& type : node.operandType) {
                if (!first) os << ",";
                os << type;
                first = false;
            }
			os << "|";

            // resultValue
            first = true;
            for (const auto& val : node.resultValue) {
                if (!first) os << ",";
                os << val;
                first = false;
            }
            os << "|";

            // resultType
            first = true;
            for (const auto& type : node.resultType) {
                if (!first) os << ",";
                os << type;
                first = false;
            }

            return os;
        }

        // Input stream operator
        std::istream& operator>>(std::istream& is, DomainFlowNode& node) {
            std::string line;
            if (!std::getline(is, line)) {
                is.setstate(std::ios::failbit);
                return is;
            }

            std::istringstream iss(line);
            std::string segment;

            // name
            if (!std::getline(iss, node.name, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
			// operator
			if (!std::getline(iss, segment, '|')) {
				is.setstate(std::ios::failbit);
				return is;
			}
			std::istringstream(segment) >> node.opType;

            // depth
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            std::istringstream(segment) >> node.depth;

            // operandType
            node.operandType.clear();
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            if (!segment.empty()) {
                std::istringstream input_ss(segment);
                std::string input;
                while (std::getline(input_ss, input, ',')) {
                    node.operandType.push_back(input);
                }
            }

            // resultValue
            node.resultValue.clear();
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            if (!segment.empty()) {
                std::istringstream val_ss(segment);
                std::string val;
                while (std::getline(val_ss, val, ',')) {
                    node.resultValue.push_back(val);
                }
            }

            // resultType
            node.resultType.clear();
            if (!std::getline(iss, segment)) {  // Last field, no delimiter at end
                is.setstate(std::ios::failbit);
                return is;
            }
            if (!segment.empty()) {
                std::istringstream type_ss(segment);
                std::string type;
                while (std::getline(type_ss, type, ',')) {
                    node.resultType.push_back(type);
                }
            }



            return is;
        }

	    // the Domain Flow Graph node type capturing the TensorProduct operation
	    struct TensorProduct : public DomainFlowNode {
		    TensorProduct(const std::string& name) : DomainFlowNode(DomainFlowOperator::MATMUL, name) {}
		    ~TensorProduct() {}
	    };
	    // the Domain Flow Graph node type capturing the Convolution operation
        struct Convolution : public DomainFlowNode {
            Convolution(const std::string& name) : DomainFlowNode(DomainFlowOperator::CONV2D, name) {}
            ~Convolution() {}
        };


    }
}

