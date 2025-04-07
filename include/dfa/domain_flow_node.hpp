#pragma once

// extendable graph data structure
#include <graph/graph.hpp>

namespace sw {
    namespace dfa {

		enum class DomainFlowOperator {
            CONSTANT,
			ADD,
			SUB,
			MUL,
			DIV,
			MATMUL,
            FC,
			CONV2D,
			MAXPOOL2D,
			AVGPOOL2D,
			UNKNOWN
		};
        // Output stream operator
        std::ostream& operator<<(std::ostream& os, DomainFlowOperator dfo) {
            switch (dfo) {
            case DomainFlowOperator::CONSTANT:   os << "CONSTANT";   break;
            case DomainFlowOperator::ADD:        os << "ADD";        break;
            case DomainFlowOperator::SUB:        os << "SUB";        break;
            case DomainFlowOperator::MUL:        os << "MUL";        break;
            case DomainFlowOperator::DIV:        os << "DIV";        break;
            case DomainFlowOperator::MATMUL:     os << "MATMUL";     break;
            case DomainFlowOperator::FC:         os << "FC";         break;
            case DomainFlowOperator::CONV2D:     os << "CONV2D";     break;
            case DomainFlowOperator::MAXPOOL2D:  os << "MAXPOOL2D";  break;
            case DomainFlowOperator::AVGPOOL2D:  os << "AVGPOOL2D";  break;
            case DomainFlowOperator::UNKNOWN:    os << "UNKNOWN";    break;
            default: throw std::invalid_argument("Unknown DomainFlowOperator value");
            }
            return os;
        }

        // Input stream operator
        std::istream& operator>>(std::istream& is, DomainFlowOperator& dfo) {
            std::string token;
            if (!std::getline(is, token, '|')) {  // Assuming '|' as delimiter from previous format
                is.setstate(std::ios::failbit);
                return is;
            }

            if (token == "CONSTANT")    dfo = DomainFlowOperator::CONSTANT;
            else if (token == "ADD")    dfo = DomainFlowOperator::ADD;
            else if (token == "SUB")    dfo = DomainFlowOperator::SUB;
            else if (token == "MUL")    dfo = DomainFlowOperator::MUL;
            else if (token == "DIV")    dfo = DomainFlowOperator::DIV;
            else if (token == "MATMUL") dfo = DomainFlowOperator::MATMUL;
            else if (token == "FC")     dfo = DomainFlowOperator::FC;
            else if (token == "CONV2D") dfo = DomainFlowOperator::CONV2D;
            else if (token == "MAXPOOL2D") dfo = DomainFlowOperator::MAXPOOL2D;
            else if (token == "AVGPOOL2D") dfo = DomainFlowOperator::AVGPOOL2D;
            else if (token == "UNKNOWN")   dfo = DomainFlowOperator::UNKNOWN;
            else {
                is.setstate(std::ios::failbit);
                throw std::invalid_argument("Invalid DomainFlowOperator string: " + token);
            }

            return is;
        }

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
            std::string getName() const noexcept { return name; }
            int getDepth() const noexcept { return depth; }
			std::size_t getNrInputs() const noexcept { return operandType.size(); }
			std::size_t getNrOutputs() const noexcept { return resultType.size(); }
            std::string getResultValue(std::size_t idx) const { if (idx < resultValue.size()) return resultValue[idx]; else return "out of bounds"; }
            std::string getResultType(std::size_t idx) const noexcept { if (idx < resultType.size()) return resultType[idx]; else return "out of bounds"; }
        };


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

