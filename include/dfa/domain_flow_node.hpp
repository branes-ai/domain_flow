#pragma once

// extendable graph data structure
#include <graph/graphlib.hpp>
#include <dfa/domain_flow_operator.hpp>
#include <dfa/tensor_spec_parser.hpp>
#include <dfa/domain_of_computation.hpp>
#include <dfa/index_point.hpp>
#include <dfa/schedule.hpp>
// helper
#include <dfa/arithmetic_complexity.hpp>

namespace sw {
    namespace dfa {

        // the Domain Flow Graph node type

        struct DomainFlowNode {
            using ConstraintCoefficientType = int;

            DomainFlowOperator opType;                      // domain flow operator type
            std::string name;                               // source dialect name
            std::map<std::size_t, std::string> operandType; // slotted string version of mlir::Type
            std::map<std::size_t, std::string> resultValue; // slotted string version of mlir::Value: typically too verbose
            std::map<std::size_t, std::string> resultType; // slotted string version of mlir::Type
			std::map<std::string, std::string> attribute;   // attributes of the operation, key/value pair where the value is encoded as a string
            int depth;                                      // depth of 0 represents a data source
			DomainOfComputation<int> doc;                   // domain of computation for the operator
			Schedule<int> tau;                              // tau represents the execution schedule for the operator

        public:
            // Constructor to initialize the node with just a string of the operator
            DomainFlowNode() 
                : opType{ DomainFlowOperator::UNKNOWN }, name{ "undefined" }, 
                operandType{}, resultValue{}, resultType{}, depth { 0 },
				doc{}, tau{} {}
            DomainFlowNode(const std::string& name) 
                : opType{ DomainFlowOperator::UNKNOWN }, name{ name }, 
                operandType{}, resultValue{}, resultType{}, depth{ 0 },
                doc{}, tau{} {}
            DomainFlowNode(DomainFlowOperator opType, const std::string& name) 
                : opType{ opType }, name{ name }, 
                operandType{}, resultValue{}, resultType{}, depth{ 0 },
                doc{}, tau{} {
            }

            // Modifiers
            void clear() noexcept {
                opType = DomainFlowOperator::UNKNOWN;
                name.clear();
                operandType.clear();
                resultValue.clear();
                resultType.clear();
                attribute.clear();
                depth = 0;
                doc.clear();
                tau.clear();
            }
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
            bool isOperator() const noexcept {
                bool bIsOperator = true;
                switch (opType) {
                case DomainFlowOperator::UNKNOWN:
                case DomainFlowOperator::CONSTANT:
                case DomainFlowOperator::FUNCTION:
                case DomainFlowOperator::FUNCTION_ARGUMENT:
				case DomainFlowOperator::FUNCTION_RETURN:
                    bIsOperator = false;
                    break;
                }
				return bIsOperator;
            }
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
			const std::map<std::string, std::string>& getAttributes() const noexcept { return attribute; }
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
                        if (!calculateMatmulShape(tensor1.shape, tensor2.shape, result)) {
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
                        auto tensorIn = parseTensorType(getOperandType(0));
						auto tensorOut = parseTensorType(getResultType(0));
                        if (tensorIn.empty() || tensorOut.empty()) {
                            std::cerr << "DomainFlowNode getArithmeticComplexity: invalid clamp arguments: ignoring clamp operator" << std::endl;
                            break;
                        }
						// assuming the input and output tensors are the same structure
                        std::uint64_t count{ 1 };
                        for (auto& dim : tensorIn.shape) {
                            count *= dim;
                        }
                        stats = { "Clamp cmp", tensorIn.elementType, 2*count };
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

					    auto imageIn = parseTensorType(getOperandType(0));
						auto imageOut = parseTensorType(getResultType(0));
						if (imageIn.empty() || imageOut.empty()) {
							std::cerr << "DomainFlowNode getArithmeticComplexity: invalid reduce_sum arguments: ignoring reduce_sum operator" << std::endl;
							break;
						}
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
        
			// instantiate the domain of computation for the operator
			// The Domain of Computation consists of a set of constraints that define the convex hull
			// and the tensor confluences that associate input and output tensor slices 
            // to specific faces of the convex hull
			void instantiateDomain() {
                doc.clear();
				// push the input tensor specification into the domain of computation
				for (auto& op : operandType) {
					doc.addInput(op.first, op.second);
				}
                // push the input tensor specification into the domain of computation
				for (auto& op : resultType) {
					doc.addOutput(op.first, op.second);
				}
				// interpret the DomainFlowOperator and select a parallel algorithm
                doc.elaborateDomainOfComputation(opType);

			}
            PointSet<ConstraintCoefficientType> convexHullPointSet() const noexcept { return doc.convexHullPointSet(); }
            ConvexHull<ConstraintCoefficientType> convexHull() const noexcept { return doc.convexHull(); }
			ConfluenceSet<ConstraintCoefficientType> confluences() const noexcept { return doc.confluences(); }
			ConstraintSet<ConstraintCoefficientType> constraints() const noexcept { return doc.constraints(); }

            void instantiateIndexSpace() noexcept {
                // generate the constraints that define the domain of computation for the operator
                doc.elaborateConstraintSet(opType);
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

