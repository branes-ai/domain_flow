#include <iostream>
#include <iomanip>
#include <filesystem>

#include "mlir/Parser/Parser.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/Support/SourceMgr.h"

// compilation
// g++ -std=c++17 -I/path/to/mlir/include -L/path/to/mlir/lib -lmlir_IR -lmlir_Dialect -lmlir_Parser -lmlir_Support -o linalg_importer linalg_importer.cpp


int main(int argc, char* argv[]) {
    mlir::MLIRContext context;
    llvm::SourceMgr sourceMgr;


    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-mlirbc-file>\n";
        return 1;
    }

    std::string filepath = argv[1];
    std::cout << "Working directory: " << std::filesystem::current_path() << '\n';
    //std::filesystem::current_path(std::filesystem::temp_directory_path()); // (3)
    //std::cout << "Current path is " << std::filesystem::current_path() << '\n';

    // Load your MLIR source into sourceMgr here.
    mlir::OwningOpRef<mlir::ModuleOp> module =
        mlir::parseSourceFile<mlir::ModuleOp>(sourceMgr, &context);
    if (!module) {
        // Handle parsing error.
        return 1;
    }

    // Use the parsed module as needed.

    return 0;
}

