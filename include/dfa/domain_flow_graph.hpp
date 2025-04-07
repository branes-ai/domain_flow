#pragma once

// extendable graph data structure
#include <graph/graph.hpp>

namespace sw {
    namespace dfa {

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

