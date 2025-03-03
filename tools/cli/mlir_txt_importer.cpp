#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <filesystem>

#include "mlir/IR/Verifier.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"  // For func::FuncDialect
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include "mlir/Parser/Parser.h"

#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"


// compilation
// g++ -std=c++17 -I/path/to/mlir/include -L/path/to/mlir/lib -lMLIRIR -lMLIRDialect -lMLIRParser -o mlir_importer mlir_importer.cpp

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
}


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

    return EXIT_SUCCESS;
}

