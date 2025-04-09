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

//#include "mlir/IR/Operation.h"
//#include "mlir/IR/Value.h"
//#include "mlir/IR/Block.h"
//#include "mlir/IR/Attributes.h"
//#include "llvm/Support/raw_ostream.h"

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
    DomainFlowGraph g(argv[1]); // Deep Learning graph
    processModule(g, *module);

    // Print the graph
    //std::cout << g << std::endl;

	// Report operator statistics
	reportOperatorStats(g);

    // Report arithmetic complexity
    reportArithmeticComplexity(g);

    // Report numerical complexity
    reportNumericalComplexity(g);

    return EXIT_SUCCESS;
}
