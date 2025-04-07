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
    MLIRTosaDialect
    MLIRMemRefDialect 
    MLIRFuncToLLVM
)


######################################################################################
# Create the mlir symbol table walker
set(DFA_MLIR_WALK_RESULTS mlir_walk_results)
set(DFA_MLIR_WALK_RESULTS_SRC mlir_walk_results.cpp)
add_mlir_tool(${DFA_MLIR_WALK_RESULTS} 
	${DFA_MLIR_WALK_RESULTS_SRC}
	PARTIAL_SOURCES_INTENDED

	DEPENDS
	${LIBS}
)
target_link_libraries(${DFA_MLIR_WALK_RESULTS} ${LIBS})
llvm_update_compile_flags(${DFA_MLIR_WALK_RESULTS})
mlir_check_all_link_libraries(${DFA_MLIR_WALK_RESULTS})
set_target_properties(${DFA_MLIR_WALK_RESULTS} PROPERTIES FOLDER "dfa/tools/tosa")


######################################################################################
# Create the TOSA parser executable
set(DFA_TOSA_PARSER tosa_parser)
set(DFA_TOSA_PARSER_SRC tosa_parser.cpp)
add_mlir_tool(${DFA_TOSA_PARSER} 
	${DFA_TOSA_PARSER_SRC}
	PARTIAL_SOURCES_INTENDED

	DEPENDS
	${LIBS}
)
target_link_libraries(${DFA_TOSA_PARSER} ${LIBS})
llvm_update_compile_flags(${DFA_TOSA_PARSER})
mlir_check_all_link_libraries(${DFA_TOSA_PARSER})
set_target_properties(${DFA_TOSA_PARSER} PROPERTIES FOLDER "dfa/tools/tosa")


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
set_target_properties(${DFA_TOSA_IMPORTER} PROPERTIES FOLDER "dfa/tools/tosa")


######################################################################################
# Create TOSA graph executor executable
set(DFA_TOSA_EXECUTOR tosa_executor)
set(DFA_TOSA_EXECUTOR_SRC tosa_executor.cpp)
add_mlir_tool(${DFA_TOSA_EXECUTOR} 
    ${DFA_TOSA_EXECUTOR_SRC}
    PARTIAL_SOURCES_INTENDED
    
    DEPENDS
    ${LIBS}
)
target_link_libraries(${DFA_TOSA_EXECUTOR} ${LIBS})
llvm_update_compile_flags(${DFA_TOSA_EXECUTOR})
mlir_check_all_link_libraries(${DFA_TOSA_EXECUTOR})
set_target_properties(${DFA_TOSA_EXECUTOR} PROPERTIES FOLDER "dfa/tools/tosa")


######################################################################################
# Create a Domain Flow TOSA analyzer tool
set(DFA_TOSA_ANALYZER analyzer)
set(DFA_TOSA_ANALYZER_SRC analyzer.cpp)
add_mlir_tool(${DFA_TOSA_ANALYZER} 
    ${DFA_TOSA_ANALYZER_SRC}
    PARTIAL_SOURCES_INTENDED
    
    DEPENDS
    ${LIBS}
)
target_link_libraries(${DFA_TOSA_ANALYZER} ${LIBS})
llvm_update_compile_flags(${DFA_TOSA_ANALYZER})
mlir_check_all_link_libraries(${DFA_TOSA_ANALYZER})
set_target_properties(${DFA_TOSA_ANALYZER} PROPERTIES FOLDER "dfa/tools/tosa")
