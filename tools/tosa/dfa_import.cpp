#if WIN32
#pragma warning(disable : 4244 4267 4996)
#endif

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include <iostream>
#include <iomanip>

#include <dfa/dfa.hpp>
#include <dfa/dfa_mlir.hpp>


int main(int argc, char **argv) {
    using namespace sw::dfa;
    // Ensure an MLIR file is provided as input.
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <MLIR file>\n";
        return 1;
    }

    // Create an MLIR context and register the TOSA dialect.
    mlir::MLIRContext context;
    mlir::DialectRegistry registry;
    registry.insert<mlir::tosa::TosaDialect>();
    registry.insert<mlir::func::FuncDialect>();
    context.appendDialectRegistry(registry);

    // Parse the provided MLIR file.
    auto module = mlir::parseSourceFile<mlir::ModuleOp>(argv[1], &context);
    if (!module) {
        std::cerr << "Failed to parse MLIR file: " << argv[1] << "\n";
        return 1;
    }

    // Walk through the operations in the module and parse them
    DomainFlowGraph dfg(argv[1]); // Deep Learning graph
    processModule(dfg, *module);

    dfg.save(std::cout);

    auto nodeOut = DomainFlowOperator("matmul", 1).addOperand("tensor<2x2xf32>").addOperand("tensor<2x2xf32>").addResult("result_0", "tensor<2x2xf32>");
    std::cout << "NODE: " << nodeOut << '\n';
    std::stringstream ss;
	ss << nodeOut;
	DomainFlowOperator nodeIn;
    ss >> nodeIn;
	std::cout << "NODE: " << nodeIn << '\n';


    DomainFlow df;
    df.flow = 5;
    df.stationair = true;
    df.shape = "tensor<1x2x3>";
	df.scalarSizeInBits = 32;
    df.schedule = { 1, 2, 3 };

    std::ostringstream oss;
    oss << df;
    std::cout << oss.str() << "\n";  // Outputs: 5|true|tensor<1x2x3xf32>|32|1,2,3

    // Reading
    DomainFlow df2;
    std::istringstream iss("3|false|tensor<4x5xi32>|32|0,1");
    iss >> df2;
    // df2 now contains: flow=3, stationair=false, shape="tensor<4x5xi32>", scalarSizeInBits=32, schedule={0,1}
	std::cout << df2 << "\n";  // Outputs: 3|false|tensor<4x5xi32>|32|0,1
    

    dfg.graph.save("test.dfa");
	DomainFlowGraph dfg2("serialized graph");
	dfg2.graph.load("test.dfa");
	std::cout << dfg2 << std::endl;

    return EXIT_SUCCESS;
}
