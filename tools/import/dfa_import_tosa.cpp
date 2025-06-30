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


int main(int argc, char** argv) {
    using namespace sw::dfa;
    // Ensure an MLIR file is provided as input.
    if (argc < 2) {
        std::cerr << "Missing input file:\n\nUsage: " << argv[0] << " <MLIR file>\n";
        return EXIT_SUCCESS;  // return success to support CI
    }

    std::string dataFileName{ argv[1] };
    if (!std::filesystem::exists(dataFileName)) {
		// search for the file in the data directory
        try {
            dataFileName = generateDataFile(argv[1]);
            std::cout << "Data file : " << dataFileName << std::endl;
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return EXIT_SUCCESS;  // return success to support CI
        }
    }

    // Create an MLIR context and register the TOSA dialect.
    mlir::MLIRContext context;
    mlir::DialectRegistry registry;
    registry.insert<mlir::tosa::TosaDialect>();
    registry.insert<mlir::func::FuncDialect>();
    context.appendDialectRegistry(registry);

    // Parse the provided MLIR file.
    auto moduleRef = mlir::parseSourceFile<mlir::ModuleOp>(dataFileName, &context);
    if (!moduleRef) {
        std::cerr << "Failed to parse MLIR file: " << dataFileName << "\n";
        return 1;
    }

    // Get a reference to the ModuleOp
    mlir::ModuleOp module = *moduleRef;

    // Walk through the operations in the module and parse them
    DomainFlowGraph dfg(dataFileName); // Deep Learning graph
    processModule(dfg, module);
	std::cout << dfg << std::endl;

	// Save the graph to a file
    std::string dfgFilename = replaceExtension(dataFileName, ".mlir", ".dfg");
    std::cout << "Original filename: " << dataFileName << std::endl;
    std::cout << "New filename: " << dfgFilename << std::endl;

    dfg.save(dfgFilename);

    // report on the operator statistics
    reportOperatorStats(dfg);

    reportArithmeticComplexity(dfg);
	reportNumericalComplexity(dfg);

    return EXIT_SUCCESS;
}


/*
The mlir::parseSourceFile<mlir::ModuleOp> returns an mlir::OwningOpRef<mlir::ModuleOp>, 
which is a smart pointer-like type that owns the module data structure. The processModule 
function expects a non-const reference to an mlir::ModuleOp (mlir::ModuleOp&), and you cannot 
directly bind the dereferenced OwningOpRef to a non-const reference because dereferencing 
it produces a temporary (rvalue).

To fix this, you need to convert the OwningOpRef<mlir::ModuleOp> into an lvalue of 
type mlir::ModuleOp that processModule can bind to. Here is how to do that:

    // Get a reference to the ModuleOp
    mlir::ModuleOp module = *moduleRef;

Explanation of the Issue:

mlir::parseSourceFile<mlir::ModuleOp>(dataFileName, &context) returns an mlir::OwningOpRef<mlir::ModuleOp>, 
which manages the lifetime of the parsed module.

When you write:
    auto module = mlir::parseSourceFile<mlir::ModuleOp>(dataFileName, &context), 
module is an OwningOpRef<mlir::ModuleOp>.

Dereferencing module with *module gives you an mlir::ModuleOp, but it’s a temporary (rvalue) 
because OwningOpRef’s dereference operator returns a reference to its managed object, 
which cannot be bound to a non-const reference (mlir::ModuleOp&).
The compiler error indicates that processModule expects an lvalue of type mlir::ModuleOp&, 
but you’re passing an rvalue.

Solution
You need to extract the mlir::ModuleOp from the OwningOpRef and ensure it’s an lvalue. 
Since OwningOpRef owns the operation, you can use its release() method to take ownership 
of the raw mlir::ModuleOp and manage its lifetime manually, or you can keep the OwningOpRef 
and pass a reference to the underlying ModuleOp correctly.

*/