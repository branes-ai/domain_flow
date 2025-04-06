#if WIN32
#pragma warning(disable : 4244 4267 4996)
#endif

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/Error.h"

#include "mlir/IR/AsmState.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Bytecode/BytecodeReader.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"


using namespace mlir;
using namespace llvm;

// Command line options
static cl::opt<std::string> inputFilename(
    cl::Positional, cl::desc("<input bytecode file>"), cl::init("-"));

static cl::opt<std::string> outputFilename(
    "o", cl::desc("Output text filename"), cl::value_desc("filename"), 
    cl::init("-"));


// usage: ./mlir_bc_converter input.mlir.bc -o output.mlir
int main(int argc, char **argv) {
    // Initialize LLVM and parse command line
    InitLLVM y(argc, argv);
    cl::ParseCommandLineOptions(argc, argv, "MLIR Bytecode to Text Converter\n");

    // Create an MLIR context
    MLIRContext context;
    context.loadAllAvailableDialects();

    // Prepare input and output streams
    std::string errorMessage;
    auto input = openInputFile(inputFilename, &errorMessage);
    if (!input) {
        llvm::errs() << "Error opening input bytecode file: " << errorMessage << '\n';
        return 1;
    }

    auto output = openOutputFile(outputFilename, &errorMessage);
    if (!output) {
        llvm::errs() << "Error opening output text file: " << errorMessage << '\n';
        return 1;
    }

    // Read the bytecode file
    // Create a SourceMgr
    auto sourceMgr = std::make_shared<SourceMgr>();

    // Add the input buffer to the SourceMgr
    sourceMgr->AddNewSourceBuffer(std::move(input), SMLoc());

    // Prepare parser configuration
    ParserConfig config(&context);

    // Create a module operation to hold the parsed content
    OwningOpRef<ModuleOp> module = ModuleOp::create(UnknownLoc::get(&context));

    // Attempt to read the bytecode file using SourceMgr
    auto readError = readBytecodeFile(sourceMgr, &module->getBodyRegion().front(), config);
    // &module->getBodyRegion().front(): 
    // The & operator is used to take the address of the Block returned by front(), 
    // providing the required Block* argument to readBytecodeFile.

    if (failed(readError)) {
        llvm::errs() << "Failed to read bytecode file: "
            << mlir::asMainReturnCode(readError) << '\n';
        return 1;
    }

    // Print the module to the output file in human-readable text format
    module->print(output->os());
    output->keep();

    return 0;
}


/*

Differences between the two bytecode reading approaches:

MemoryBufferRef Approach:

Takes a raw memory buffer directly
Simpler, more direct method
Less context and error reporting
Typically used when you have a raw memory buffer and want a quick read


SourceMgr Approach:

Uses a more sophisticated source management system
Provides better error tracking and source location information
Allows for more complex parsing scenarios
Supports multiple input sources and more detailed error reporting
More flexible for complex parsing needs



The SourceMgr version is generally preferred when you need:

Detailed error reporting
Source location tracking
Potential for multiple input sources
More robust parsing infrastructure
 */