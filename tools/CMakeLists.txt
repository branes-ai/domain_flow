add_subdirectory(cli)
add_subdirectory(mlir)
add_subdirectory(rdg)

if(DFATOOLS_VISUALIZATION)
# viz
add_subdirectory(viz)
endif()

