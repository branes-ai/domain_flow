cmake_minimum_required(VERSION 3.28)
project(isl_schedule_tree_demo)

# Find ISL (assumes it's installed system-wide)
find_path(ISL_INCLUDE_DIR NAMES isl/ctx.h)
find_library(ISL_LIBRARY NAMES isl)

if(NOT ISL_INCLUDE_DIR OR NOT ISL_LIBRARY)
    #message(FATAL_ERROR "Could not find ISL library or headers")
    message(ERROR " Could not find ISL library or headers: ignoring ISL targets")
else()
    # Add ISL targets

    # C target
    # set C standard
    set(CMAKE_C_STANDARD 99)

    add_executable(schedule_tree_demo_c schedule_tree_demo.c)
    target_include_directories(schedule_tree_demo_c PRIVATE ${ISL_INCLUDE_DIR})
    target_link_libraries(schedule_tree_demo_c PRIVATE ${ISL_LIBRARY})


    # C++ target
    # set C++ standard
    set(CMAKE_CXX_STANDARD 20)

    add_executable(schedule_tree_demo_cpp schedule_tree_demo.cpp)
    target_include_directories(schedule_tree_demo_cpp PRIVATE ${ISL_INCLUDE_DIR})
    target_link_libraries(schedule_tree_demo_cpp PRIVATE ${ISL_LIBRARY})

endif()
