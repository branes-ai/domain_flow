#pragma once

// extendable graph data structure
#include <graph/graphlib.hpp>
#include <dfa/domain_flow_operator.hpp>
#include <dfa/tensor_spec_parser.hpp>
#include <dfa/arithmetic_complexity.hpp>

namespace sw {
    namespace dfa {

        struct shapeAnalysisResults {
            int64_t batchSize, m, k, n;
            std::string errMsg;
            void clear() {
                batchSize = 0;
                m = 0;
                k = 0;
                n = 0;
                errMsg.clear();
            }
            void setError(const std::string& errMsg) { this->errMsg = errMsg; }
            uint64_t macOps() const {
                return batchSize * m * n * k;
            }
        };

        /// <summary>
        /// shape analysis: for numpy matmul, the last two dimensions are the matmul dimensions, leading dimensions are batch dimensions
        /// (b1, b2, ..., bn, m, k) * (b1, b2, ..., bn, k, n) yields (b1, b2, ..., bn, m, n)
        /// </summary>
        /// <param name="shape0"></param>
        /// <param name="shape1"></param>
        /// <returns>true if analysis determines there are no inconsistencies, false otherwise</returns>
        bool calculateMatmulComplexity(const std::vector<int>& lhsShape, const std::vector<int>& rhsShape, shapeAnalysisResults& result) {
            result.clear();
            // Ensure rhs tensor has at least 2 dimensions
            if (rhsShape.size() < 2) {
                result.setError("Right tensor must have at least 2 dimensions.");
                return false;
            }

            // Ensure lhs tensor has at least 1 dimension
            if (lhsShape.empty()) {
                result.setError("Left tensor must have at least 1 dimension.");
                return false;
            }

            // Extract k, n from rhs tensor
            int k1 = rhsShape[rhsShape.size() - 2]; // Rows of right matrix
            int n = rhsShape[rhsShape.size() - 1];  // Columns of right matrix

            // Determine batch dimensions from shape1
            size_t batch_dims = rhsShape.size() - 2; // Number of batch dimensions

            // Determine if shape0 is a vector or matrix
            bool is_vector = (lhsShape.size() == batch_dims + 1);
            bool is_matrix = (lhsShape.size() == batch_dims + 2);

            // Validate shape0 size
            if (!is_vector && !is_matrix) {
                result.setError("Left tensor has incompatible number of dimensions.");
                return false;
            }

            // Extract m, k from lhs tensor
            int m, k0;
            if (lhsShape.size() == 1) {
                // Vector case: lhs shape = (batch_size,)
                m = 1;
                k0 = lhsShape[0];
            }
            else {
                // Matrix case: lhs shape = (..., m, k)
                m = lhsShape[lhsShape.size() - 2];
                k0 = lhsShape[lhsShape.size() - 1];
            }

            // Validate that reduction dimensions match
            if (k0 != k1) {
                result.setError("Inner dimensions must match: k0 != k1");
                return false;
            }
            int k = k0;

            // Check batch dimensions
            uint64_t batch_size = 1;
            for (size_t i = 0; i < batch_dims; ++i) {
                if (i >= lhsShape.size()) {
                    result.setError("Left tensor has too few dimensions for batch.");
                    return false;
                }
                if (lhsShape[i] != rhsShape[i]) {
                    result.setError("Batch dimensions must match.");
                    return false;
                }
                if (lhsShape[i] <= 0) {
                    result.setError("Dimensions must be positive.");
                    return false;
                }
                batch_size *= static_cast<uint64_t>(lhsShape[i]);
            }

            // Validate matrix/vector dimensions
            if (m <= 0 || k <= 0 || n <= 0) {
                result.setError("Matrix dimensions must be positive.");
                return false;
            }

            // Total MAC operations = batch_size * m * n * k
            result.batchSize = batch_size;
            result.m = m;
            result.k = k;
            result.n = n;
            return true;
        }

        // the Domain Flow Graph node type
        struct DomainFlowNode {
            DomainFlowOperator opType;                      // domain flow operator type
            std::string name;                               // source dialect name
            std::map<std::size_t, std::string> operandType; // slotted string version of mlir::Type
            std::map<std::size_t, std::string> resultValue; // slotted string version of mlir::Value: typically too verbose
            std::map<std::size_t, std::string> resultType; // slotted string version of mlir::Type
			std::map<std::string, std::string> attribute;   // attributes of the operation, key/value pair where the value is encoded as a string
            int depth;                                      // depth of 0 represents a data source

            // Constructor to initialize the node with just a string of the operator
            DomainFlowNode() : opType{ DomainFlowOperator::UNKNOWN }, name{ "undefined" }, operandType{}, resultValue{}, resultType{}, depth { 0 } {}
            DomainFlowNode(const std::string& name) : opType{ DomainFlowOperator::UNKNOWN }, name{ name }, operandType{}, resultValue{}, resultType{}, depth{ 0 } {}
            DomainFlowNode(DomainFlowOperator opType, const std::string& name) : opType{ opType }, name{ name }, operandType{}, resultValue{}, resultType{}, depth{ 0 } {}

            // Modifiers
            void setOperator(DomainFlowOperator opType, std::string name) { this->opType = opType;  this->name = name; }
            void setDepth(int d) { depth = d; }

            DomainFlowNode& addOperand(const std::size_t slot, const std::string& typeStr) {
                operandType[slot] =(typeStr);
                return *this;
            }
			DomainFlowNode& addAttribute(const std::string& name, const std::string& value) {
				attribute[name] = value;
				return *this;
			}
            DomainFlowNode& addResult(const std::size_t slot, const std::string& valueStr, const std::string& typeStr) {
                resultValue[slot] = (valueStr);
                resultType[slot] = (typeStr);
                return *this;
            }

            // selectors
			DomainFlowOperator getOperator() const noexcept { return opType; }
            std::string getName() const noexcept { return name; }
            int getDepth() const noexcept { return depth; }
            // input operand API
			std::size_t getNrInputs() const noexcept { return operandType.size(); }
            std::string getOperandType(std::size_t slot) const { 
                auto it = operandType.find(slot); 
                if (it != operandType.end()) {
                    return it->second;
                }
                else {
                    return "n/a";
                }
            }
			// attribute API
            std::size_t getNrAttributes() const noexcept { return attribute.size(); }
			std::map<std::string, std::string> getAttributes() const noexcept { return attribute; }
            std::string getAttribute(const std::string& name) const noexcept { 
                auto it = attribute.find(name); 
                if (it != attribute.end()) {
                    return it->second;
                }
                else {
                    return "n/a";
                }
            }
            std::string getAttributeValue(const std::string& name) const {
                auto it = attribute.find(name);
                if (it != attribute.end()) {
                    return it->second;
                }
                else {
                    return "n/a";
                }
            }
			// output result API
            std::size_t getNrOutputs() const noexcept { return resultType.size(); }
            std::string getResultValue(std::size_t slot) const noexcept { 
                auto it = resultValue.find(slot);  
                if (it != resultValue.end()) {
                    return it->second;
                }
                else {
                    return "n/a";
                }
            }
            std::string getResultType(std::size_t slot) const noexcept { auto it = resultType.find(slot); if (it != resultType.end()) return it->second; else return "n/a"; }
        
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

                    // TODO: do we validate the validity of the operand pair?
                        auto tensorInfo = parseTensorType(getOperandType(0));
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
                        auto tensorInfo = parseTensorType(getOperandType(0));
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
                        auto tensorInfo = parseTensorType(getOperandType(0));
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
                        TensorTypeInfo tensor1 = parseTensorType(getOperandType(0));
                        TensorTypeInfo tensor2 = parseTensorType(getOperandType(1));
                        if (tensor1.empty() || tensor2.empty()) {
                            std::cerr << "DomainFlowNode getArithmeticComplexity: invalid matmul arguments: ignoring matmul operator" << std::endl;
                            break;
                        }

                        shapeAnalysisResults result;
                        if (!calculateMatmulComplexity(tensor1.shape, tensor2.shape, result)) {
                            std::cerr << "DomainFlowNode getArithmeticComplexity: " << result.errMsg << std::endl;
                            break;
                        }
                        else {
                            std::uint64_t count = result.macOps();

                            stats = { "Fused Multiply", tensor1.elementType, count };
                            work.push_back(stats);
                            stats = { "Add", tensor1.elementType, count };
                            work.push_back(stats);
                        }

                        // TBD: calculate conversion cost
#ifdef CALCULATE_CONVERSION_COST
                        // a, b, c used to be dimension of lhs tensor
                        // d, e, f the shape of the rhs tensor
                        if (tensor1.elementType != tensor2.elementType) {
                            int sizeOf1 = a * b * c;
							int sizeOf2 = d * e * f;
                            // inspect types to see which tensor needs to be converted by inspecting the type conversion rules
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
#endif

                    }
					break;
                case DomainFlowOperator::CONV2D:
                    {
    					TensorTypeInfo input, kernel, bias, result;
						size_t nrOperands = operandType.size();
                        switch (nrOperands) {
                        case 2:
							// Conv2D with no bias
							input = parseTensorType(getOperandType(0));
							kernel = parseTensorType(getOperandType(1));
							break;
                        case 3:
							// Conv2D with bias
							input = parseTensorType(getOperandType(0));
							kernel = parseTensorType(getOperandType(1));
							bias = parseTensorType(getOperandType(2));
							break;
						default:
							std::cerr << "Error: Conv2D operation requires 2 or 3 operands" << std::endl;
							break;
                        }
                        if (resultType.size() != 1) {
                            std::cerr << "Error: Conv2D operation requires 1 result" << std::endl;
                            break;
                        }
                        result = parseTensorType(getResultType(0));
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
                            input = parseTensorType(getOperandType(0));
                            kernel = parseTensorType(getOperandType(1));
                            break;
                        case 3:
                            // Conv2D with bias
                            input = parseTensorType(getOperandType(0));
                            kernel = parseTensorType(getOperandType(1));
                            bias = parseTensorType(getOperandType(2));
                            break;
                        default:
                            std::cerr << "Error: Depthwise Conv2D operation requires 2 or 3 operands" << std::endl;
                            break;
                        }
                        if (resultType.size() != 1) {
                            std::cerr << "Error: Depthwise Conv2D operation requires 1 result" << std::endl;
                            break;
                        }
                        result = parseTensorType(getResultType(0));
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
                        auto tensorInfo = parseTensorType(getOperandType(1));
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

					    auto imageIn = parseTensorType(getOperandType(1));

						auto imageOut = parseTensorType(getResultType(0));
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
                        std::string axisStr = attribute.at(std::string("axis"));
						int axis = std::stoi(axisStr);
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

		inline bool operator==(const DomainFlowNode& lhs, const DomainFlowNode& rhs) {
			return (lhs.opType == rhs.opType) && (lhs.name == rhs.name) && (lhs.operandType == rhs.operandType) && (lhs.attribute == rhs.attribute)
				&& (lhs.resultValue == rhs.resultValue) && (lhs.resultType == rhs.resultType) && (lhs.depth == rhs.depth);
		}
        inline bool operator!=(const DomainFlowNode& lhs, const DomainFlowNode& rhs) {
            return !(lhs == rhs);
        }

        // Output stream operator
        inline std::ostream& operator<<(std::ostream& os, const DomainFlowNode& node) {
            // Format: name|operator|depth|operandType1,operandType2|resultValue1,resultValue2|resultType1,resultType2
            os << "|" << node.name << "|";
            os << node.opType << "|";
            os << node.depth << "|";

            // operandType
            bool first = true;
            for (const auto& type : node.operandType) {
                if (!first) os << ",";
                os << type.first << ':' << type.second;
                first = false;
            }
			os << "|";

            // attributes
            first = true;
            for (const auto& val : node.attribute) {
                if (!first) os << ",";
                os << val.first << ':' << val.second;
                first = false;
            }
            os << "|";

            // resultValue
            first = true;
            for (const auto& val : node.resultValue) {
                if (!first) os << ",";
                os << val.first << ':' << val.second;
                first = false;
            }
            os << "|";

            // resultType
            first = true;
            for (const auto& type : node.resultType) {
                if (!first) os << ",";
                os << type.first << ':' << type.second;
                first = false;
            }

            return os;
        }

        // Input stream operator
        inline std::istream& operator>>(std::istream& is, DomainFlowNode& node) {
            std::string line;
            if (!std::getline(is, line)) {
                is.setstate(std::ios::failbit);
                return is;
            }

            std::istringstream iss(line);
            std::string segment;

			// synchronize the segments by removing the first '|' and any white space
			if (!std::getline(iss, segment, '|')) {
				is.setstate(std::ios::failbit);
				return is;
			}
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
					std::string slot, type;
					std::size_t pos = input.find(':');
                    if (pos != std::string::npos) {
                        slot = input.substr(0, pos);
                        type = input.substr(pos + 1);
                        std::size_t slotNum = std::stoul(slot);
                        node.operandType[slotNum] = type;
                    }
                }
            }

			// attributes
			node.attribute.clear();
			if (!std::getline(iss, segment, '|')) {
				is.setstate(std::ios::failbit);
				return is;
			}
			if (!segment.empty()) {
				std::istringstream attr_ss(segment);
				std::string attr;
				while (std::getline(attr_ss, attr, ',')) {
					std::string name, value;
					std::size_t pos = attr.find(':');
					if (pos != std::string::npos) {
						name = attr.substr(0, pos);
						value = attr.substr(pos + 1);
						node.attribute[name] = value;
					}
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
					std::string slot, type;
					std::size_t pos = val.find(':');
					if (pos != std::string::npos) {
						slot = val.substr(0, pos);
						type = val.substr(pos + 1);
						std::size_t slotNum = std::stoul(slot);
						node.resultValue[slotNum] = type;
					}
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
					std::string slot, typeStr;
					std::size_t pos = type.find(':');
					if (pos != std::string::npos) {
						slot = type.substr(0, pos);
						typeStr = type.substr(pos + 1);
						std::size_t slotNum = std::stoul(slot);
						node.resultType[slotNum] = typeStr;
					}
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

