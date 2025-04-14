cmake_minimum_required(VERSION 3.20)  # Or higher

####
# Set project variables
if(NOT DEFINED DOMAINFLOW_VERSION_MAJOR)
  set(DOMAINFLOW_VERSION_MAJOR 0)
endif()
if(NOT DEFINED DOMAINFLOW_VERSION_MINOR)
  set(DOMAINFLOW_VERSION_MINOR 1)
endif()
if(NOT DEFINED DOMAINFLOW_VERSION_PATCH)
  set(DOMAINFLOW_VERSION_PATCH 1)
endif()

cmake_policy(SET CMP0048 NEW) # The project() command manages VERSION variables
cmake_policy(SET CMP0077 NEW) # option() honors normal variables

project(domain_flow 
	DESCRIPTION "A header only C++ library for domain flow architecture parallelizing compiler"
	VERSION "${DOMAINFLOW_VERSION_MAJOR}.${DOMAINFLOW_VERSION_MINOR}.${DOMAINFLOW_VERSION_PATCH}" 
	LANGUAGES C CXX ASM
	HOMEPAGE_URL "https://github.com/branes-ai/domain_flow")

# double check that the presets are coming through
message(STATUS "CMakePresets")
message(STATUS "MLIR_DIR                    :  ${MLIR_DIR}")
message(STATUS "CMAKE_TOOLCHAIN_FILE        :  ${CMAKE_TOOLCHAIN_FILE}")

# Create an interface library target as domain_flow is a header-only library
add_library(${PROJECT_NAME} INTERFACE)

# Set include directories for users of your library
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
# Set C++ standard for the library
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20) # Or 14, 17, 20, 23, etc.

# Set C++ standard (optional but recommended)
set(CMAKE_CXX_STANDARD 20)  # Or 14, 17, 20, 23, etc.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "C++20 has been enabled by default")

set(DOMAINFLOW_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

option(DOMAINFLOW_USE_FOLDERS "Enable solution folders in Visual Studio, disable for Express"   ON)
if (DOMAINFLOW_USE_FOLDERS) 
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

####
# Create the library target
set(project_library_target_name ${PROJECT_NAME})
set(PACKAGE_NAME ${PROJECT_NAME})
message(STATUS "PROJECT_NAME = ${PROJECT_NAME}")

if(NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    # we're being included as a subproject 
    add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
endif()


####
# Set environmental options for tracing, testing, and verbosity
option(DOMAINFLOW_CMAKE_TRACE "Tracing CMake results, i.e. printing variable settings." OFF)
option(DOMAINFLOW_ENABLE_TESTS "Enable the build and run of tests." ON)
option(DOMAINFLOW_VERBOSE_TESTS "Always print test output, otherwise only errors. Only relevant when tests enabled." OFF)
option(DOMAINFLOW_VISUALIZATION "Enable/disable visualization tools." OFF)

# Set the toolchain directory
#set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Path to the vcpkg.cmake")
#better set through the CMakePreset mechanism

# Must be located in root dir, doesn't work in tests
if (DOMAINFLOW_ENABLE_TESTS)
    enable_testing()
    # include(Dart)
endif()

####
# Set project wide include directory
include_directories("./include")

macro(trace_variable variable)
    if (DOMAINFLOW_CMAKE_TRACE)
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
            if (DOMAINFLOW_CMAKE_TRACE)
                message(STATUS "testing: ${test_name} ${RUNTIME_OUTPUT_DIRECTORY}/${test_name}")
            endif()
            add_test(${test_name} ${RUNTIME_OUTPUT_DIRECTORY}/${test_name})
        endif()
    endforeach (source)
endmacro (compile_all)

# the json library tests
add_subdirectory(src/json/tests)
# the base graph library tests
add_subdirectory(src/graph/tests)
# the domain flow library tests
add_subdirectory(src/dfa/tests)

# the workload generators
add_subdirectory(workloads)
# the tools
add_subdirectory(tools)
# the plotters
add_subdirectory(plots)
# the databases
add_subdirectory(databases)

# Install rules (for packaging)

# Install the headers
install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
    PATTERN ".svn" EXCLUDE
    PATTERN ".git" EXCLUDE
    PATTERN ".DS_Store" EXCLUDE
)
# Install the library target 
install(TARGETS ${DFA_TOOL_CLI} DESTINATION bin)
export(TARGETS ${PROJECT_NAME} FILE "${PROJECT_NAME}Config.cmake")
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Export targets
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE "sw::dfa::"
    DESTINATION lib/cmake/${PROJECT_NAME}
)

# Create and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/${PROJECT_NAME}-config.cmake.in 
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)
# Create and install version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY  SameMajorVersion
)
# Install the config and version files
install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)

