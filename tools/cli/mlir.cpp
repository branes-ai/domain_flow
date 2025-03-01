#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "mlir/IR/AsmState.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Bytecode/BytecodeReader.h" // Correct include for BytecodeReader
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

// Reads an MLIR bytecode file (.mlirbc) into an in-memory ModuleOp.
OwningOpRef<ModuleOp> deserializeMLIRBytecode(const std::string& filepath, MLIRContext* context) {
    // Open the file in binary mode
    auto fileBuffer = llvm::MemoryBuffer::getFile(filepath, /*IsText=*/false);
    if (!fileBuffer) {
        std::cerr << "Error: Could not open file: " << filepath << "\n";
        return nullptr;
    }

    // Create a top-level block to store the parsed module
    Block block;

    // Use the correct readBytecodeFile function
    if (failed(mlir::readBytecodeFile(fileBuffer->get()->getMemBufferRef(), &block, ParserConfig(context)))) {
        std::cerr << "Error: Failed to parse MLIR bytecode from " << filepath << "\n";
        return nullptr;
    }

    // Extract the parsed module
    auto module = dyn_cast<ModuleOp>(block.front());
    if (!module) {
        std::cerr << "Error: Failed to extract ModuleOp from parsed bytecode.\n";
        return nullptr;
    }

    // Verify the module (optional, for debugging)
    if (failed(verify(module))) {
        std::cerr << "Error: Module verification failed\n";
        module.dump();
        return nullptr;
    }

    return OwningOpRef<ModuleOp>(module);
}

// Example usage
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-mlirbc-file>\n";
        return 1;
    }

    // Initialize MLIR context
    MLIRContext context;
    context.getOrLoadDialect<BuiltinDialect>(); // Load basic dialects

    //// Deserialize the .mlirbc file
    //OwningOpRef<ModuleOp> module = deserializeMLIRBytecode(filepath, &context);
    //if (!module) {
    //    return 1;
    //}

    //// Print the module to stdout (proof it’s in memory)
    //std::cout << "Successfully deserialized module:\n";
    //module->print(llvm::outs());

    return 0;
}
