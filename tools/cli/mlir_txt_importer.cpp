#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <filesystem>

#if WIN32
#pragma warning(disable : 4244 4267)
#endif

#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include "mlir/IR/Verifier.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/IR/Region.h"
#include "mlir/IR/Block.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"  // For func::FuncDialect
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include "mlir/Transforms/Passes.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"


namespace mlir {
    OwningOpRef<ModuleOp> read_mlirbc(const std::string& filepath, MLIRContext* context) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Could not open file: " << filepath << std::endl;
            return nullptr;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        std::string buffer(size, ' ');
        file.seekg(0);
        file.read(&buffer[0], size);
        file.close();

        std::unique_ptr<llvm::MemoryBuffer> memBuffer = llvm::MemoryBuffer::getMemBuffer(
            llvm::StringRef(buffer.data(), buffer.size()), filepath, false);

        llvm::SourceMgr sourceMgr;
        sourceMgr.AddNewSourceBuffer(std::move(memBuffer), llvm::SMLoc());

        OwningOpRef<ModuleOp> module = parseSourceFile<ModuleOp>(sourceMgr, ParserConfig(context));
        if (!module) {
            std::cerr << "Error: Failed to parse MLIR bytecode from " << filepath << std::endl;
            return nullptr;
        }

        if (failed(verify(*module))) {
            std::cerr << "Error: Module verification failed" << std::endl;
            module->dump();
            return nullptr;
        }

        return module;
    }

    struct PrintNodeNamesPass : public PassWrapper<PrintNodeNamesPass, OperationPass<ModuleOp>> {
        void runOnOperation() override {
            ModuleOp module = getOperation();
            module.walk([](Operation* op) {
                llvm::outs() << op->getName() << "\n";
                });
        }
    };

    static unsigned indentLevel = 0;
    static constexpr unsigned SPACES_PER_INDENT = 2;

    static void resetIndent() { indentLevel = 0u; }
    static unsigned pushIndent() { return ++indentLevel; }
    static unsigned popIndent() { return --indentLevel; }
    static llvm::raw_ostream& printIndent() {
        return llvm::outs() << std::string(indentLevel * SPACES_PER_INDENT, ' ');
    }

    // forward declarations
    void printOperation(Operation*);
    void printBlock(Block&);
    void printRegion(Region&);

    void printOperation(Operation* op) {
        // Print the operation itself and some of its properties
        printIndent() 
            << "visiting op: '" << op->getName() 
            << "' with " << op->getNumOperands() 
            << " operands and " << op->getNumResults() << " results\n";

        // Print the operation attributes
        if (!op->getAttrs().empty()) {
            printIndent() 
                << op->getAttrs().size() << " attributes:\n";
            for (NamedAttribute attr : op->getAttrs())
                printIndent() 
                << " - '" << attr.getName() 
                << "' : '" << attr.getValue() << "'\n";
        }

        // Recurse into each of the regions attached to the operation.
        printIndent() << " " << op->getNumRegions() << " nested regions:\n";
        pushIndent();
        for (Region& region : op->getRegions())
            printRegion(region);
        popIndent();
    }

    void printBlock(Block& block) {
        // Print the block intrinsics properties (basically: argument list)
        printIndent()
            << "Block with " << block.getNumArguments() 
            << " arguments, " << block.getNumSuccessors()
            << " successors, and "
            // Note, this `.size()` is traversing a linked-list and is O(n).
            << block.getOperations().size() << " operations\n";

        // A block main role is to hold a list of Operations: let's recurse into
        // printing each operation.
        pushIndent();
        for (Operation& op : block.getOperations())
            printOperation(&op);
        popIndent();
    }

    void printRegion(Region& region) {
        // A region does not hold anything by itself other than a list of blocks.
        printIndent() 
            << "Region with " << region.getBlocks().size()
            << " blocks:\n";
        pushIndent();
        for (Block& block : region.getBlocks())
            printBlock(block);
        popIndent();
    }

    struct PrintGraphPass : public PassWrapper<PrintNodeNamesPass, OperationPass<ModuleOp>> {
        void runOnOperation() override {
            //Operation* op = getOperation();
            //printOperation(op);
            resetIndent();
            ModuleOp module = getOperation();
            module.walk([](Operation* op) {
                printOperation(op);
            });
        }
    };

} // namespace


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-mlirbc-file>\n";
        return 1;
    }

    std::string filepath = argv[1];
    std::cout << "Working directory: " << std::filesystem::current_path() << '\n';
    //std::filesystem::current_path(std::filesystem::temp_directory_path()); // (3)
    //std::cout << "Current path is " << std::filesystem::current_path() << '\n';

    mlir::MLIRContext context;
    context.getOrLoadDialect<mlir::BuiltinDialect>();
    context.getOrLoadDialect<mlir::func::FuncDialect>();  // Use func::FuncDialect instead of FuncDialect
    context.getOrLoadDialect<mlir::tosa::TosaDialect>();
    // context.allowsUnregisteredDialects();  
    // don't quite know how to use allowsUnregisteredDialects: 
    // parsing still fails when you remove a dialect, use this function, and give it a file containing
    // an unregistered dialect.

    // Load your MLIR source into sourceMgr here.
    mlir::OwningOpRef<mlir::ModuleOp> module = mlir::read_mlirbc(filepath, &context);
    if (!module) {
        // Handle parsing error.
        std::cerr << "Unable to read MLIR file: " << filepath << '\n';
        return EXIT_FAILURE;
    }

    // Use the parsed module as needed.
    std::cout << "Successfully deserialized module:\n";
    module->print(llvm::outs());

    std::cout << "\n\n\n\n";
    mlir::PassManager pm(&context);

    // Add the custom pass to print node names
    // pm.addPass(std::make_unique<mlir::PrintNodeNamesPass>());
    // Add the custom pass to print the graph
    pm.addPass(std::make_unique<mlir::PrintGraphPass>());

    // Create an empty module
    mlir::OpBuilder builder(&context);
    //mlir::ModuleOp module = mlir::ModuleOp::create(builder.getUnknownLoc());

    // Run the pass manager on the module
    llvm::outs() << "indent level : " << mlir::indentLevel << '\n';
    if (failed(pm.run(*module))) {
        llvm::errs() << "PassManager execution failed!\n";
        return 1;
    }
    llvm::outs() << "indent level : " << mlir::indentLevel << '\n'; // should be 0

    return EXIT_SUCCESS;
}

