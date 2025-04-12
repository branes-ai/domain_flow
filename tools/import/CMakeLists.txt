####
# set up environment for the import tools that rely on MLIR
message(STATUS "Setting up environment for import tools based on MLIR")

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM and MLIR packages
find_package(MLIR REQUIRED CONFIG)

message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})

# set up the CMake module environment to find cmake macros like add_mlir_tool used below
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(AddMLIR)

# LLVM/MLIR include directories
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})

# Add LLVM definitions
add_definitions(${LLVM_DEFINITIONS})

# LLVM/MLIR libraries
link_directories(${LLVM_BUILD_LIBRARY_DIR})
#link_directories(${MLIR_BUILD_LIBRARY_DIR})  # this variable is not defined

#message(STATUS "LLVM_LIBRARY_DIRS : ${LLVM_LIBRARY_DIRS}")
#message(STATUS "LLVM_BUILD_LIBRARY_DIR: ${LLVM_BUILD_LIBRARY_DIR}")
#message(STATUS "MLIR_BUILD_LIBRARY_DIR: ${MLIR_BUILD_LIBRARY_DIR}")   # this variable is undefined

# base library set
set(LIBS 
    MLIRParser 
    MLIRSupport 
    MLIRFuncDialect 
    MLIRSCFDialect
    MLIRTosaDialect
    MLIRMemRefDialect 
    MLIRFuncToLLVM
)

######################################################################################
# Create TOSA graph importer executable
set(DFA_TOSA_IMPORTER dfa-import-tosa)
set(DFA_TOSA_IMPORTER_SRC dfa_import.cpp)
add_mlir_tool(${DFA_TOSA_IMPORTER} 
    ${DFA_TOSA_IMPORTER_SRC}
    PARTIAL_SOURCES_INTENDED
    
    DEPENDS
    ${LIBS}
)
target_link_libraries(${DFA_TOSA_IMPORTER} ${LIBS})
llvm_update_compile_flags(${DFA_TOSA_IMPORTER})
mlir_check_all_link_libraries(${DFA_TOSA_IMPORTER})
set_target_properties(${DFA_TOSA_IMPORTER} PROPERTIES FOLDER "tools/importers")

