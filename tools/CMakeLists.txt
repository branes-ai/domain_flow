# domain flow graph tests
add_subdirectory(rdg)
add_subdirectory(dfg)

if(DFATOOLS_VISUALIZATION)
add_subdirectory(viz)
endif()

# MLIR dependent projects
add_subdirectory(opt)
add_subdirectory(tosa)
