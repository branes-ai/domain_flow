add_subdirectory(opt)
add_subdirectory(mlir)
add_subdirectory(tosa)
add_subdirectory(rdg)

if(DFATOOLS_VISUALIZATION)
add_subdirectory(viz)
endif()

