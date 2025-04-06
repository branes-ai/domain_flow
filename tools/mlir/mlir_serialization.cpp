#if WIN32
#pragma warning(disable : 4244 4267 4996)
#endif

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ToolOutputFile.h"

#include "mlir/IR/AsmState.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Support/FileUtilities.h"

using namespace mlir;
using namespace llvm;

// Command line options
static cl::opt<std::string> inputFilename(
    cl::Positional, cl::desc("<input mlir file>"), cl::init("-"));

static cl::opt<std::string> outputFilename(
    "o", cl::desc("Output filename"), cl::value_desc("filename"), 
    cl::init("-"));


// usage: mlir_serialization input.mlir - o output.mlir
int main(int argc, char **argv) {
    // Initialize LLVM and parse command line
    InitLLVM y(argc, argv);
    cl::ParseCommandLineOptions(argc, argv, "MLIR Serialization\n");

    // Create an MLIR context
    MLIRContext context;
    context.loadAllAvailableDialects();

    // Prepare input and output streams
    std::string errorMessage;
    auto input = openInputFile(inputFilename, &errorMessage);
    if (!input) {
        llvm::errs() << "Error opening input file: " << errorMessage << "\n";
        return 1;
    }

    auto output = openOutputFile(outputFilename, &errorMessage);
    if (!output) {
        llvm::errs() << "Error opening output file: " << errorMessage << "\n";
        return 1;
    }

    // Parse the input MLIR file
    SourceMgr sourceMgr;
    sourceMgr.AddNewSourceBuffer(std::move(input), SMLoc());

    // Parse the module
    ParserConfig config(&context);
    auto module = parseSourceFile<ModuleOp>(sourceMgr, config);
    if (!module) {
        llvm::errs() << "Failed to parse MLIR module\n";
        return 1;
    }

    // Print the module to the output file
    module->print(output->os());
    output->keep();

    return 0;
}
