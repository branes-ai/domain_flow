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

        // the Domain Flow Graph edge type
        struct DomainFlow : public sw::graph::weighted_edge<int> { // Weighted by the data flow on this link
            int flow;
            bool stationair;  // does the flow go through a memory or not
            std::string shape;  // tensor<1x2x3x4x5> as example
			int scalarSizeInBits; // size of the scalar type of the tensor in bits
            std::vector<int> schedule;  // N-D vector of the schedule

            int weight() const noexcept override { return flow; }

            void setStationarity(bool inMemory) { stationair = inMemory; }
            void setShape(std::string shape) { this->shape = shape; }
            void setSchedule(std::vector<int> schedule) { this->schedule = schedule; }

            DomainFlow() : flow{ 0 }, stationair{ true }, shape{ "1xi32" }, scalarSizeInBits{ 32 }, schedule{ {0,0,0} } {}
            DomainFlow(int flow, bool inMemory = true) : flow{ flow }, stationair{ inMemory }, shape{ "1xi32" }, scalarSizeInBits{ 32 }, schedule{ {0,0,0} } {}
			DomainFlow(int flow, bool inMemory, std::string shape, int scalarSizeInBits, std::vector<int> tau) : flow{ flow }, stationair{ inMemory }, shape{ shape }, scalarSizeInBits{ scalarSizeInBits }, schedule{ tau } {}
            ~DomainFlow() {}
        };

        // Output stream operator
        std::ostream& operator<<(std::ostream& os, const DomainFlow& df) {
            // Format: flow|stationair|shape|tau1,tau2,...
            os << df.flow << "|" << (df.stationair ? "true" : "false") << "|" << df.shape << "|" << df.scalarSizeInBits << "|";

            // schedule
            bool first = true;
            for (const auto& sched : df.schedule) {
                if (!first) os << ",";
                os << sched;
                first = false;
            }

            return os;
        }

        // Input stream operator
        std::istream& operator>>(std::istream& is, DomainFlow& df) {
            std::string line;
            if (!std::getline(is, line)) {
                is.setstate(std::ios::failbit);
                return is;
            }

            std::istringstream iss(line);
            std::string segment;

            // flow (weight)
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            std::istringstream(segment) >> df.flow;

            // stationair
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            df.stationair = (segment == "true");

            // shape
            if (!std::getline(iss, df.shape, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }

			// scalarSizeInBits
			if (!std::getline(iss, segment, '|')) {
				is.setstate(std::ios::failbit);
				return is;
			}
			std::istringstream(segment) >> df.scalarSizeInBits;

            // scheduling vector
            df.schedule.clear();
            if (!std::getline(iss, segment)) {  // Last field, no delimiter at end
                is.setstate(std::ios::failbit);
                return is;
            }
            if (!segment.empty()) {
                std::istringstream sched_ss(segment);
                std::string sched_val;
                while (std::getline(sched_ss, sched_val, ',')) {
                    int val;
                    std::istringstream(sched_val) >> val;
                    df.schedule.push_back(val);
                }
            }

            return is;
        }

        // Definition of the Domain Flow graph
		struct DomainFlowGraph {
            std::string name;
			sw::graph::directed_graph<DomainFlowOperator, DomainFlow> graph{};
			std::vector<sw::graph::nodeId_t> source;
			std::vector<sw::graph::nodeId_t> sink;

			DomainFlowGraph(std::string name) : name{ name } {}
			DomainFlowGraph(std::string name, sw::graph::directed_graph<DomainFlowOperator, DomainFlow> graph,
				std::vector<sw::graph::nodeId_t> source, std::vector<sw::graph::nodeId_t> sink) :
				name{ name }, graph{ graph }, source{ source }, sink{ sink } {
			}
			~DomainFlowGraph() {}

            std::map<std::string, int> operatorStats() const {
				std::map<std::string, int> opCount;
                for (auto& node : graph.nodes()) {
                    auto op = node.second.getName();
                    if (opCount.find(op) == opCount.end()) {
                        opCount[op] = 1;
                    }
                    else {
                        opCount[op]++;
                    }
                }
                return opCount;
            }

			std::ostream& save(std::ostream& ostr) const {
				ostr << "Domain Flow Graph: " << name << "\n";
                graph.save(ostr);
				return ostr;
			}



		};
		std::ostream& operator<<(std::ostream& ostr, const DomainFlowGraph& g) {
			ostr << "Domain Flow Graph: " << g.name << "\n";
            g.graph.save(ostr);
			return ostr;
		}

		std::istream& operator>>(std::istream& istr, DomainFlowGraph& g) {
			std::string line;
			if (!std::getline(istr, line)) {
				istr.setstate(std::ios::failbit);
				return istr;
			}
			g.name = line;
			// Read the graph from the input stream
			g.graph.load(istr);
			return istr;
		}

    }
}

