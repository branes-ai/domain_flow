find_package(CGAL CONFIG REQUIRED)
if(CGAL_FOUND)
    find_package(CGAL COMPONENTS Qt6)
    if(CGAL_Qt6_FOUND)
        set(DFA_SIMPLE_OBJECT simple)
        include_directories(${CGAL_INCLUDE_DIRS})
        add_definitions(-DCGAL_USE_BASIC_VIEWER)
        add_executable(${DFA_SIMPLE_OBJECT} simple.cpp)
        target_include_directories(${DFA_SIMPLE_OBJECT} PUBLIC
            ${DFATOOLS_ROOT}/include
        )
        target_link_libraries(${DFA_SIMPLE_OBJECT} PRIVATE CGAL::CGAL CGAL::CGAL_Basic_viewer)
        set_target_properties(${DFA_SIMPLE_OBJECT} PROPERTIES FOLDER "viz")
    else()
        message(FATAL_ERROR "CGAL Qt6 not found")
    endif(CGAL_Qt6_FOUND)

else()
    message(FATAL_ERROR "CGAL not found")
endif()

#find_package(Qt6 COMPONENTS Core Gui Widgets 3DCore 3DExtras 3DInput 3DRender REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
if(CGAL_FOUND)
    set(DFA_QT3D_TEST qt3d_test)
    add_executable(${DFA_QT3D_TEST} qt3d_test.cpp)
    target_include_directories(${DFA_QT3D_TEST} PUBLIC
        ${DFATOOLS_ROOT}/include
    )
    #target_link_libraries(${DFA_QT3D_TEST} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::3DCore Qt6::3DExtras Qt6::3DInput Qt6::3DRender)
    target_link_libraries(${DFA_QT3D_TEST} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
    set_target_properties(${DFA_QT3D_TEST} PROPERTIES FOLDER "viz")

else()
    message(FATAL_ERROR "CGAL 3DCore not found")
endif()
