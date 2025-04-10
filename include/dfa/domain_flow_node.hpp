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
            std::map<std::string, int> attribute;
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
			DomainFlowNode& addAttribute(const std::string& name, const int value) {
				attribute[name] = value;
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
                        stats = { "Element-wise Add", tensorInfo.elementType, count };
                        work.push_back(stats);
                    }
                    break;
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
                        stats = { "Element-wise Sub", tensorInfo.elementType, count };
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
                        stats = { "Element-wise Mul", tensorInfo.elementType, count };
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
                    {
    					TensorTypeInfo input, kernel, bias, result;
						size_t nrOperands = operandType.size();
                        switch (nrOperands) {
                        case 2:
							// Conv2D with no bias
							input = parseTensorType(operandType[0]);
							kernel = parseTensorType(operandType[1]);
							break;
                        case 3:
							// Conv2D with bias
							input = parseTensorType(operandType[0]);
							kernel = parseTensorType(operandType[1]);
							bias = parseTensorType(operandType[2]);
							break;
						default:
							std::cerr << "Error: Conv2D operation requires 2 or 3 operands" << std::endl;
							break;
                        }
                        if (resultType.size() != 1) {
                            std::cerr << "Error: Conv2D operation requires 1 result" << std::endl;
                            break;
                        }
                        result = parseTensorType(resultType[0]);
                        // double check we have the proper 4D tensors
                        if (input.shape.size() != 4 || kernel.shape.size() != 4 || result.shape.size() != 4) {
                            std::cerr << "Error: Conv2D operation requires 4D tensors" << std::endl;
                            break;
                        }
                        int batch = input.shape[0];
                        int inHeight = input.shape[1];
                        int inWidth = input.shape[2];
                        int inputChannels = input.shape[3];

                        int outputChannels = kernel.shape[0];
                        int kernelHeight = kernel.shape[1];
                        int kernelWidth = kernel.shape[2];
                        int kernelChannels = kernel.shape[3];

                        int batch2 = result.shape[0];
                        int height = result.shape[1];
                        int width = result.shape[2];
                        int outputChannels2 = result.shape[3];

                        int kernelSize = kernelHeight * kernelWidth * kernelChannels;
                        uint64_t kernelMuls = kernelSize * outputChannels;
                        uint64_t kernelAdds = (kernelSize - 1) * outputChannels;

                        // check if the batch size between input and output are correct
                        if (batch != batch2) {
                            std::cerr << "Error: Conv2D operation requires the same batch size for input and output" << std::endl;
                            break;
                        }
                        uint64_t conv2DMuls = batch * height * width * outputChannels * kernelSize;
                        uint64_t conv2DAdds = batch * height * width * outputChannels * (kernelSize - 1);
                        stats = { "Conv2D-Mul", result.elementType, conv2DMuls };
                        work.push_back(stats);
                        stats = { "Conv2D-Add", result.elementType, conv2DAdds };
                        work.push_back(stats);
						// ignoring the bias for now
                    }
					break;
                case DomainFlowOperator::DEPTHWISE_CONV2D:
                    {
                        TensorTypeInfo input, kernel, bias, result;
                        size_t nrOperands = operandType.size();
                        switch (nrOperands) {
                        case 2:
                            // Conv2D with no bias
                            input = parseTensorType(operandType[0]);
                            kernel = parseTensorType(operandType[1]);
                            break;
                        case 3:
                            // Conv2D with bias
                            input = parseTensorType(operandType[0]);
                            kernel = parseTensorType(operandType[1]);
                            bias = parseTensorType(operandType[2]);
                            break;
                        default:
                            std::cerr << "Error: Depthwise Conv2D operation requires 2 or 3 operands" << std::endl;
                            break;
                        }
                        if (resultType.size() != 1) {
                            std::cerr << "Error: Depthwise Conv2D operation requires 1 result" << std::endl;
                            break;
                        }
                        result = parseTensorType(resultType[0]);
                        // double check we have the proper 4D tensors
                        if (input.shape.size() != 4 || kernel.shape.size() != 4 || result.shape.size() != 4) {
                            std::cerr << "Error: Depthwise Conv2D operation requires 4D tensors" << std::endl;
                            break;
                        }
                        int batch = input.shape[0];
                        int inHeight = input.shape[1];
                        int inWidth = input.shape[2];
                        int inputChannels = input.shape[3];

                        
                        int kernelHeight = kernel.shape[0];
                        int kernelWidth = kernel.shape[1];
                        int inputChannels2 = kernel.shape[2];
                        int channelMultiplier = kernel.shape[3];

                        int batch2 = result.shape[0];
                        int height = result.shape[1];
                        int width = result.shape[2];
                        int outputChannels = result.shape[3];

                        int kernelSize = kernelHeight * kernelWidth * channelMultiplier;

                        // check if the batch size between input and output are correct
                        if (batch != batch2) {
                            std::cerr << "Error: Conv2D operation requires the same batch size for input and output" << std::endl;
                            break;
                        }
                        uint64_t dwConv2DMuls = batch * height * width * outputChannels * kernelSize;
                        uint64_t dwConv2DAdds = batch * height * width * outputChannels * (kernelSize - 1);
                        stats = { "DW-Conv2D-Mul", result.elementType, dwConv2DMuls };
                        work.push_back(stats);
                        stats = { "DW-Conv2D-Add", result.elementType, dwConv2DAdds };
                        work.push_back(stats);
                        // ignoring the bias for now
                    }
                    break;
                case DomainFlowOperator::CLAMP:
                    {
                        // Clamp operation
                        //    %out = tosa.clamp %in : tensor<12x6xf32> -> tensor<12x6xf32>
                        auto tensorInfo = parseTensorType(operandType[0]);
                        std::uint64_t count{ 1 };
                        for (auto& dim : tensorInfo.shape) {
                            count *= dim;
                        }
                        stats = { "Clamp cmp", tensorInfo.elementType, 2*count };
						work.push_back(stats);
                    }
                    break;
                case DomainFlowOperator::REDUCE_SUM:
                    {
					    // Reduce Sum operation
					    //    %out = tosa.reduce_sum %image {axis = 1 : i32} : (tensor<?x7x7x1280xf32>) -> tensor<?x1x7x1280xf32>
                        // Input Tensor (%image): `?x7x7x1280xf32` (Unknown batch size, 7x7 spatial dimensions, 1280 channels, float32 data type)
                        //  Axis: `1 : i32` (Reduce along the second axis, which is the height dimension)
                        // The `reduce_sum` operator sums the elements of the input tensor along the specified axis.
                        // In this case, we're summing along the height dimension (axis 1). This means that for each batch, width, and channel, we'll sum the 7 elements along the height.

					    auto imageIn = parseTensorType(operandType[0]);

						auto imageOut = parseTensorType(resultType[0]);
                        // structure of vector
                        // batchSize x height
                        // batchSize x height x width
                        // batchSize x height x width x channels
                        std::vector<int> shape(4, 1);
                        for (size_t i = 0; i < imageOut.shape.size(); ++i) {
						    shape[i] = imageOut.shape[0];
                        }

                        // For each element in the output tensor, we need to sum (Axis) nr of elements from the input tensor.
						// in our example case of summing over Axis 1, we would need to sum 7 elements from the input tensor.

						// TBD: find the axis from the attributes
                        auto axis = attribute.at(std::string("axis"));
						int axisDim = imageIn.shape[axis];
                        int count{ axisDim };
                        for (auto& dim : shape) {
                            count *= dim;
                        }
					    
					    stats = { "Reduce Sum Add", imageOut.elementType, 2 * count };
						work.push_back(stats);
                    }
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

