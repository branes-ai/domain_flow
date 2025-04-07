#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;


    DomainFlowGraph dfg("my-test-graph");
	dfg.addNode(DomainFlowOperator::ADD, "test.Add");
	dfg.addNode("test.Sub");
	dfg.addNode("test.Matmul");

    dfg.graph.save(std::cout);

    dfg.graph.save("test.dfg");
    DomainFlowGraph dfg2("serialized graph");
    dfg2.graph.load("test.dfg");
    std::cout << dfg2 << std::endl;

    return EXIT_SUCCESS;
}
