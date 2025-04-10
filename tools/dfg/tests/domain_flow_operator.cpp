#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>

void testOperatorSerialization(sw::dfa::DomainFlowOperator op) {
	using namespace sw::dfa;

	std::stringstream ss;
	ss << op;
	// std::cout << "Serialized operator: " << ss.str() << '\n';
	DomainFlowOperator opIn;
	ss >> opIn;
	if (op != opIn) {
	 std::cerr << "Domain Flow Operator " << op << " failed to serialize\n";
	}
	
}

int main() {
    using namespace sw::dfa;

	// enumerate all the Domain Flow operators in our database
	bool bSuccess = true;
	for (auto op : AllDomainFlowOperators()) {
		std::cout << "Processing operator: " << op << std::endl;
		testOperatorSerialization(op);
		
	}

	if (bSuccess) {
		std::cout << "All operators processed successfully.\n";
	}
	else {
		std::cerr << "Some operators failed to process correctly.\n";
	}

    return (bSuccess ? EXIT_SUCCESS : EXIT_FAILURE);
}
