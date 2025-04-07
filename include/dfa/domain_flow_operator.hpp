#pragma once

// extendable graph data structure
#include <graph/graph.hpp>

namespace sw {
    namespace dfa {

        // the Domain Flow Graph node type
        struct DomainFlowOperator {
            std::string operatorName;
            int depth;   // 0 is a source
            std::vector<std::string> operandType;      // string version of mlir::Type
            std::vector<std::string> resultValue;    // string version of mlir::Value: typically too verbose
            std::vector<std::string> resultType;     // string version of mlir::Type


            // Constructor to initialize the node with just a string of the operator
			DomainFlowOperator() : operatorName{ "undefined" }, depth{ 0 } {}
            DomainFlowOperator(const std::string& name) : operatorName{ name }, depth{ 0 } {}
			DomainFlowOperator(const std::string& name, int d) : operatorName{ name }, depth{ d } {}
            void setDepth(int d) { depth = d; }
            void setOperator(std::string name) { this->operatorName = name; }

            DomainFlowOperator& addOperand(const std::string& typeStr) {
                operandType.push_back(typeStr);
                return *this;
            }
            DomainFlowOperator& addResult(const std::string& valueStr, const std::string& typeStr) {
                resultValue.push_back(valueStr);
                resultType.push_back(typeStr);
                return *this;
            }

            std::string getName() const noexcept { return operatorName; }
            int getDepth() const noexcept { return depth; }
			std::size_t getNrInputs() const noexcept { return operandType.size(); }
			std::size_t getNrOutputs() const noexcept { return resultType.size(); }
            std::string getResultValue(std::size_t idx) const noexcept { return resultValue[idx]; }
            std::string getResultType(std::size_t idx) const noexcept { return resultType[idx]; }
        };
        // Output stream operator
        std::ostream& operator<<(std::ostream& os, const DomainFlowOperator& dfo) {
            // Format: operatorName|depth|operandType1,operandType2|resultValue1,resultValue2|resultType1,resultType2
            os << dfo.operatorName << "|" << dfo.depth << "|";

            // operandType
            bool first = true;
            for (const auto& type : dfo.operandType) {
                if (!first) os << ",";
                os << type;
                first = false;
            }
			os << "|";

            // resultValue
            first = true;
            for (const auto& val : dfo.resultValue) {
                if (!first) os << ",";
                os << val;
                first = false;
            }
            os << "|";

            // resultType
            first = true;
            for (const auto& type : dfo.resultType) {
                if (!first) os << ",";
                os << type;
                first = false;
            }

            return os;
        }

        // Input stream operator
        std::istream& operator>>(std::istream& is, DomainFlowOperator& dfo) {
            std::string line;
            if (!std::getline(is, line)) {
                is.setstate(std::ios::failbit);
                return is;
            }

            std::istringstream iss(line);
            std::string segment;

            // operatorName
            if (!std::getline(iss, dfo.operatorName, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }

            // depth
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            std::istringstream(segment) >> dfo.depth;

            // operandType
            dfo.operandType.clear();
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            if (!segment.empty()) {
                std::istringstream input_ss(segment);
                std::string input;
                while (std::getline(input_ss, input, ',')) {
                    dfo.operandType.push_back(input);
                }
            }

            // resultValue
            dfo.resultValue.clear();
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            if (!segment.empty()) {
                std::istringstream val_ss(segment);
                std::string val;
                while (std::getline(val_ss, val, ',')) {
                    dfo.resultValue.push_back(val);
                }
            }

            // resultType
            dfo.resultType.clear();
            if (!std::getline(iss, segment)) {  // Last field, no delimiter at end
                is.setstate(std::ios::failbit);
                return is;
            }
            if (!segment.empty()) {
                std::istringstream type_ss(segment);
                std::string type;
                while (std::getline(type_ss, type, ',')) {
                    dfo.resultType.push_back(type);
                }
            }



            return is;
        }

	// the Domain Flow Graph node type capturing the TensorProduct operation
	struct TensorProduct : public DomainFlowOperator {
		TensorProduct(std::string name) : DomainFlowOperator(name) {}
		~TensorProduct() {}
	};
	// the Domain Flow Graph node type capturing the Convolution operation
        struct Convolution : public DomainFlowOperator {
            Convolution(std::string name) : DomainFlowOperator(name) {}
            ~Convolution() {}
        };


    }
}

