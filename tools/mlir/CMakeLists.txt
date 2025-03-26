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
# regular Cmake doesn't work
# 'add_executable(${DFA_MLIR_TXT_PARSER} mlir_serialization.cpp)
set(DFA_MLIR_TXT_PARSER_SRC mlir_serialization.cpp)
add_mlir_tool(${DFA_MLIR_TXT_PARSER} 
	${DFA_MLIR_TXT_PARSER_SRC}
	PARTIAL_SOURCES_INTENDED

	DEPENDS
	${LIBS}
)
target_link_libraries(${DFA_MLIR_TXT_PARSER} ${LIBS})
llvm_update_compile_flags(${DFA_MLIR_TXT_PARSER})
mlir_check_all_link_libraries(${DFA_MLIR_TXT_PARSER})
set_target_properties(${DFA_MLIR_TXT_PARSER} PROPERTIES FOLDER "dfa/tools/mlir")

# Create the mlir text parser executable
set(DFA_MLIR_BC_CONVERTER mlir_bc_converter)
#add_executable(${DFA_MLIR_BC_CONVERTER} mlir_bc_converter.cpp)
set(DFA_MLIR_BC_CONVERTER_SRC mlir_bc_converter.cpp)
append(LIBS 
    MLIRBytecode
)
add_mlir_tool(${DFA_MLIR_BC_CONVERTER} 
	${DFA_MLIR_BC_CONVERTER_SRC}
	PARTIAL_SOURCES_INTENDED

	DEPENDS
	${LIBS}
)
target_link_libraries(${DFA_MLIR_BC_CONVERTER} ${LIBS})
llvm_update_compile_flags(${DFA_MLIR_BC_CONVERTER})
mlir_check_all_link_libraries(${DFA_MLIR_BC_CONVERTER})
set_target_properties(${DFA_MLIR_BC_CONVERTER} PROPERTIES FOLDER "dfa/tools/mlir")
