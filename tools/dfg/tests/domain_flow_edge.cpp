#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>

int main() {
    using namespace sw::dfa;

    // testing the API of the Domain Flow edge abstraction
    DomainFlowEdge df;
    df.flow = 5;
    df.stationair = true;
    df.shape = "tensor<1x2x3>";
	df.scalarSizeInBits = 32;
	df.srcSlot = 0;
	df.dstSlot = 1;
    df.schedule = { 1, 2, 3 };

    std::ostringstream oss;
    oss << df;
    std::cout << oss.str() << "\n";  // Outputs: 5|true|tensor<1x2x3xf32>|32|1,2,3

    // Reading
    DomainFlowEdge df2;
    std::istringstream iss("3|false|tensor<4x5xi32>|32|0|1|0,1");
    iss >> df2;
    // df2 now contains: flow=3, stationair=false, shape="tensor<4x5xi32>", scalarSizeInBits=32, srcSlot=0, dstSlot=1 schedule={0,1}
	std::cout << df2 << "\n";  // Outputs: 3|false|tensor<4x5xi32>|32|0|1|0,1
    
    return EXIT_SUCCESS;
}
