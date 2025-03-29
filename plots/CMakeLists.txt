
find_package(Matplot++ CONFIG REQUIRED)

if (Matplot++_FOUND)

    message(STATUS "Matplot++__DIR: ${Matplot++_DIR}")
    
    set(PARETO pareto)
    add_executable(${PARETO} pareto/test.cpp)
    #target_include_directories(${PARETO} PRIVATE ${Matplot++_INCLUDE_DIR})
    target_link_libraries(${PARETO} PRIVATE Matplot++::cimg  Matplot++::matplot)
    set_target_properties(${PARETO} PROPERTIES FOLDER "dfa/plots/pareto")

    set(SCATTER scatter)
    add_executable(${SCATTER} scatter/test.cpp)
    #target_include_directories(${SCATTER} PRIVATE ${Matplot++_INCLUDE_DIR})
    target_link_libraries(${SCATTER} PRIVATE Matplot++::cimg  Matplot++::matplot)
    set_target_properties(${SCATTER} PROPERTIES FOLDER "dfa/plots/scatter")
endif()
