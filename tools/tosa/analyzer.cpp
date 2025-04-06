#if WIN32
#pragma warning(disable : 4244 4267 4996)
#endif

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include <iostream>

//#include "mlir/IR/Operation.h"
//#include "mlir/IR/Value.h"
//#include "mlir/IR/Block.h"
//#include "mlir/IR/Attributes.h"
//#include "llvm/Support/raw_ostream.h"

#include <dfa/dfa.hpp>
#include <dfa/dfa_mlir.hpp>


int main(int argc, char **argv) {
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
    sw::dfa::graph::directed_graph<sw::dfa::TosaOperator, sw::dfa::DataFlow> gr; // Deep Learning graph
    sw::dfa::processModule(gr, *module);

    // Print the graph
    //std::cout << gr << std::endl;

	// Print the nodes and their properties
	for (auto& node : gr.nodes()) {
		std::cout << "Node ID: " << node.first << ": " << node.second << " In degree: " << gr.in_degree(node.first) << " Out degree: " << gr.out_degree(node.first) << '\n';
	}

    return 0;
}
