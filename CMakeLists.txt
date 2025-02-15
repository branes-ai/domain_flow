cmake_minimum_required(VERSION 3.10)  # Or higher

project(dfa-tools)

# Set C++ standard (optional but recommended)
set(CMAKE_CXX_STANDARD 20)  # Or 14, 17, 20, 23, etc.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DFATOOLS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

option(DFATOOLS_USE_FOLDERS "Enable solution folders in Visual Studio, disable for Express"   ON)
if (DFATOOLS_USE_FOLDERS) 
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

####
# Create the library target
set(project_library_target_name ${PROJECT_NAME})
set(PACKAGE_NAME ${PROJECT_NAME})

####
# Set environmental options for tracing, testing, and verbosity
option(DFATOOLS_CMAKE_TRACE "Tracing CMake results, i.e. printing variable settings." OFF)
option(DFATOOLS_ENABLE_TESTS "Enable the build and run of tests." ON)
option(DFATOOLS_VERBOSE_TESTS "Always print test output, otherwise only errors. Only relevant when tests enabled." OFF)

macro(trace_variable variable)
    if (DFATOOLS_CMAKE_TRACE)
        message(STATUS "${variable} = ${${variable}}")
    endif()
endmacro()

####
# macro to read all cpp files in a directory
# and create a test target for that cpp file
macro (compile_all testing prefix library folder)
    # cycle through the sources
    # For the according directories, we assume that each cpp file is a separate test
    # so, create a executable target and an associated test target
    foreach (source ${ARGN})
        get_filename_component (test ${source} NAME_WE)
        string(REPLACE " " ";" new_source ${source})
        set(test_name ${prefix}_${test})
        #message(STATUS "Add test ${test_name} from source ${new_source}.")
        add_executable (${test_name} ${new_source})
        target_link_libraries(${test_name} ${library})

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
set(DFA_LIBRARY dfatools)

# the domain flow library
add_subdirectory(src/dfa)

message(STATUS "DFA_LIBRARY: ${DFA_LIBRARY}")

# the tools
add_subdirectory(tools)

# Optional: Install rules (for packaging)
install(TARGETS ${DFA_TOOL_CLI} DESTINATION bin)
install(TARGETS ${DFA_LIBRARY} DESTINATION lib)
install(FILES include/dfa/dfa.hpp DESTINATION include) # Install the header


