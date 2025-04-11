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
#include <algorithm>

#include <dfa/dfa.hpp>
#include <dfa/dfa_mlir.hpp>
#include <util/data_file.hpp>


int main(int argc, char **argv) {
    using namespace sw::dfa;
    // Ensure an MLIR file is provided as input.
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <MLIR file>\n";
        return 1;
    }

    std::string dataFileName{};
    try {
        dataFileName = generateDataFile(argv[1]);
		std::cout << "Data file : " << dataFileName << std::endl;
    }
	catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

    // Create an MLIR context and register the TOSA dialect.
    mlir::MLIRContext context;
    mlir::DialectRegistry registry;
    registry.insert<mlir::tosa::TosaDialect>();
    registry.insert<mlir::func::FuncDialect>();
    context.appendDialectRegistry(registry);

    // Parse the provided MLIR file.
    auto module = mlir::parseSourceFile<mlir::ModuleOp>(dataFileName, &context);
    if (!module) {
        std::cerr << "Failed to parse MLIR file: " << dataFileName << "\n";
        return 1;
    }

    // Walk through the operations in the module and parse them
    DomainFlowGraph dfg(dataFileName); // Deep Learning graph
    processModule(dfg, *module);

    std::string dfgFilename = replaceExtension(dataFileName, ".mlir", ".dfg");
    std::cout << "Original filename: " << dataFileName << std::endl;
    std::cout << "New filename: " << dfgFilename << std::endl;

    dfg.graph.save(dfgFilename);

    // report on the operator statistics
    reportOperatorStats(dfg);

    reportArithmeticComplexity(dfg);
	reportNumericalComplexity(dfg);

    return EXIT_SUCCESS;
}
