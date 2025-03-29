cmake_minimum_required(VERSION 3.10)  # Or higher

####
# Set project variables
if(NOT DEFINED DFATOOLS_VERSION_MAJOR)
  set(DFATOOLS_VERSION_MAJOR 0)
endif()
if(NOT DEFINED DFATOOLS_VERSION_MINOR)
  set(DFATOOLS_VERSION_MINOR 1)
endif()
if(NOT DEFINED DFATOOLS_VERSION_PATCH)
  set(DFATOOLS_VERSION_PATCH 1)
endif()

cmake_policy(SET CMP0048 NEW) # The project() command manages VERSION variables
cmake_policy(SET CMP0077 NEW) # option() honors normal variables

project(domain_flow 
	DESCRIPTION "A header only C++ library for domain flow architecture parallelizing compiler"
	VERSION "${DFATOOLS_VERSION_MAJOR}.${DFATOOLS_VERSION_MINOR}.${DFATOOLS_VERSION_PATCH}" 
	LANGUAGES C CXX ASM
	HOMEPAGE_URL "https://github.com/branes-ai/dfa-dynamics")

# double check that the presets are coming through
message(STATUS "CMakePresets")
message(STATUS "MLIR_DIR                    :  ${MLIR_DIR}")
message(STATUS "CMAKE_TOOLCHAIN_FILE        :  ${CMAKE_TOOLCHAIN_FILE}")

# dfa is a header-only library
add_library(${PROJECT_NAME} INTERFACE)

# Set C++ standard (optional but recommended)
set(CMAKE_CXX_STANDARD 20)  # Or 14, 17, 20, 23, etc.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "C++20 has been enabled by default")

set(DFATOOLS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

option(DFATOOLS_USE_FOLDERS "Enable solution folders in Visual Studio, disable for Express"   ON)
if (DFATOOLS_USE_FOLDERS) 
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

####
# Create the library target
set(project_library_target_name ${PROJECT_NAME})
set(PACKAGE_NAME ${PROJECT_NAME})
message(STATUS "PROJECT_NAME = ${PROJECT_NAME}")

####
# Set environmental options for tracing, testing, and verbosity
option(DFATOOLS_CMAKE_TRACE "Tracing CMake results, i.e. printing variable settings." OFF)
option(DFATOOLS_ENABLE_TESTS "Enable the build and run of tests." ON)
option(DFATOOLS_VERBOSE_TESTS "Always print test output, otherwise only errors. Only relevant when tests enabled." OFF)
option(DFATOOLS_VISUALIZATION "Enable/disable visualization tools." OFF)

# Set the toolchain directory
#set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Path to the vcpkg.cmake")

# Must be located in root dir, doesn't work in tests
if (DFATOOLS_ENABLE_TESTS)
    enable_testing()
    # include(Dart)
endif()

####
# Set project wide include directory
include_directories("./include")

macro(trace_variable variable)
    if (DFATOOLS_CMAKE_TRACE)
        message(STATUS "${variable} = ${${variable}}")
    endif()
endmacro()

####
# macro to read all cpp files in a directory
# and create a test target for that cpp file
macro (compile_all testing prefix folder)
    # cycle through the sources
    # For the according directories, we assume that each cpp file is a separate test
    # so, create a executable target and an associated test target
    foreach (source ${ARGN})
        get_filename_component (test ${source} NAME_WE)
        string(REPLACE " " ";" new_source ${source})
        set(test_name ${prefix}_${test})
        message(STATUS "Add test ${test_name} from source ${new_source}.")
        add_executable (${test_name} ${new_source})

        #add_custom_target(valid SOURCES ${SOURCES})
        set_target_properties(${test_name} PROPERTIES FOLDER ${folder})
        if (${testing} STREQUAL "true")
            if (DFATOOLS_CMAKE_TRACE)
                message(STATUS "testing: ${test_name} ${RUNTIME_OUTPUT_DIRECTORY}/${test_name}")
            endif()
            add_test(${test_name} ${RUNTIME_OUTPUT_DIRECTORY}/${test_name})
        endif()
    endforeach (source)
endmacro (compile_all)

# why do we need to set it here and can't we set it in the library CMakeLists.txt file?
# when I set it in the library CMakeLists.txt file, the value doesn't show up in the tools CMakeLists.txt file
set(DFA_LIBRARY dfalib)

# the domain flow library
add_subdirectory(src/dfa)

message(STATUS "DFA_LIBRARY: ${DFA_LIBRARY}")

# the tools
add_subdirectory(tools)
# the plotters
add_subdirectory(plots)
# the databases
add_subdirectory(databases)

# Optional: Install rules (for packaging)
install(TARGETS ${DFA_TOOL_CLI} DESTINATION bin)
install(TARGETS ${DFA_LIBRARY} DESTINATION lib)
install(FILES include/dfa/dfa.hpp DESTINATION include) # Install the header, we need to add all the other headers too TBD


