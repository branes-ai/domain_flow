cmake_minimum_required(VERSION 3.20)
project(MLIR_Serialization LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find MLIR package
find_package(MLIR REQUIRED CONFIG)
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# MLIR include directories
include_directories(${MLIR_INCLUDE_DIRS})
include_directories(${LLVM_INCLUDE_DIRS})

# Add LLVM definitions
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

# base library set
set(LIBS 
    MLIRParser 
    MLIRSupport 
    MLIRFuncDialect 
    MLIRSCFDialect 
    MLIRMemRefDialect 
    MLIRFuncToLLVM
)

# Create the mlir text parser executable
set(DFA_MLIR_TXT_PARSER mlir_serialization)
add_executable(${DFA_MLIR_TXT_PARSER} mlir_serialization.cpp)
set_target_properties(${DFA_MLIR_TXT_PARSER} PROPERTIES FOLDER "dfa/tools/mlir")
# Link against MLIR libraries
target_link_libraries(${DFA_MLIR_TXT_PARSER} ${LIBS})

# Create the mlir text parser executable
set(DFA_MLIR_BC_CONVERTER mlir_bc_converter)
add_executable(${DFA_MLIR_BC_CONVERTER} mlir_bc_converter.cpp)
set_target_properties(${DFA_MLIR_BC_CONVERTER} PROPERTIES FOLDER "dfa/tools/mlir")
append(LIBS 
    MLIRBytecode
)
# Link against MLIR libraries
target_link_libraries(${DFA_MLIR_BC_CONVERTER} ${LIBS})
