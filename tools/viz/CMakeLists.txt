message(STATUS "Configuring cgal basic viewer")
find_package(CGAL CONFIG REQUIRED)
if(CGAL_FOUND)
    message(STATUS "CGAL found")
    find_package(CGAL COMPONENTS Qt6)
    message(STATUS "Qt6_FOUND      : ${Qt6_FOUND}")
    message(STATUS "CGAL_Qt6_FOUND : ${CGAL_Qt6_FOUND}")
    if(CGAL_Qt6_FOUND)
    	message(STATUS "CGAL Qt6 found")
        set(DFA_SIMPLE_OBJECT simple)
        include_directories(${CGAL_INCLUDE_DIRS})
        add_definitions(-DCGAL_USE_BASIC_VIEWER)
        add_executable(${DFA_SIMPLE_OBJECT} simple.cpp)
        target_include_directories(${DFA_SIMPLE_OBJECT} PUBLIC
            ${DFATOOLS_ROOT}/include
        )
        target_link_libraries(${DFA_SIMPLE_OBJECT} PRIVATE CGAL::CGAL CGAL::CGAL_Basic_viewer)
        set_target_properties(${DFA_SIMPLE_OBJECT} PROPERTIES FOLDER "dfa/tools/viz")
    else()
        message(STATUS "CGAL Qt6 not found: disabling CGAL applications")
    endif(CGAL_Qt6_FOUND)

else()
    message(STATUS "CGAL not found: disabling CGAL applications")
endif()

#CMake Error at D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake:893 (_find_package):
#  Found package configuration file:
#
#    D:/dev/vcpkg/installed/x64-windows/share/Qt6/Qt6Config.cmake
#
#  but it set Qt6_FOUND to FALSE so package "Qt6" is considered to be NOT
#  FOUND.  Reason given by package:
#
#  Failed to find required Qt component "3DCore".
#
#  Expected Config file at
#  "D:/dev/vcpkg/installed/x64-windows/share/Qt63DCore/Qt63DCoreConfig.cmake"
#  does NOT exist
#
#
#  Configuring with --debug-find-pkg=Qt63DCore might reveal details why the
#  package was not found.
#
#  Configuring with -DQT_DEBUG_FIND_PACKAGE=ON will print the values of some
#  of the path variables that find_package uses to try and find the package.
#    find_package search path values and other context for the last package that was not found:
#
#    CMAKE_MODULE_PATH: D:/dev/vcpkg/installed/x64-windows/share/cgal;D:/dev/vcpkg/installed/x64-windows/share/Qt6;D:/dev/vcpkg/installed/x64-windows/share/Qt6/3rdparty/extra-cmake-modules/find-modules;D:/dev/vcpkg/installed/x64-windows/share/Qt6/3rdparty/kwin;D:/dev/vcpkg/installed/x64-windows/share/Qt6;D:/dev/vcpkg/installed/x64-windows/share/Qt6/3rdparty/extra-cmake-modules/find-modules;D:/dev/vcpkg/installed/x64-windows/share/Qt6/3rdparty/kwin
#    CMAKE_PREFIX_PATH: D:/dev/vcpkg/installed/x64-windows/debug;D:/dev/vcpkg/installed/x64-windows
#    $ENV{CMAKE_PREFIX_PATH}: 
#    CMAKE_FIND_ROOT_PATH: D:/dev/vcpkg/installed/x64-windows/debug;D:/dev/vcpkg/installed/x64-windows
#    _qt_additional_packages_prefix_paths: 
#    _qt_additional_host_packages_prefix_paths: 
#    _qt_cmake_dir: D:/dev/vcpkg/installed/x64-windows/share
#    QT_HOST_PATH: 
#    Qt6HostInfo_DIR: 
#    Qt6_DIR: D:/dev/vcpkg/installed/x64-windows/share/Qt6
#    CMAKE_TOOLCHAIN_FILE: D:\dev\vcpkg/scripts/buildsystems/vcpkg.cmake
#    CMAKE_FIND_ROOT_PATH_MODE_PACKAGE: 
#    CMAKE_SYSROOT: 


message(STATUS "Configuring cgal Qt3D test")
find_package(Qt6 COMPONENTS Core Gui Widgets 3DCore 3DExtras 3DInput 3DRender)
#find_package(Qt6 COMPONENTS Core Gui Widgets 3DCore 3DExtras 3DInput 3DRender REQUIRED)
# if you use REQUIRED CMake will error out instead of simply removing the application from the build
message(STATUS "Qt6_FOUND      : ${Qt6_FOUND}")
message(STATUS "CGAL_Qt6_FOUND : ${CGAL_Qt6_FOUND}")
if(Qt6_FOUND)
    message(STATUS "CGAL Qt6 3DCore and 3DRender found")
    set(DFA_QT3D_TEST qt3d_test)
    add_executable(${DFA_QT3D_TEST} qt3d_test.cpp)
    target_include_directories(${DFA_QT3D_TEST} PUBLIC
        ${DFATOOLS_ROOT}/include
    )
    target_link_libraries(${DFA_QT3D_TEST} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::3DCore Qt6::3DExtras Qt6::3DInput Qt6::3DRender)
    set_target_properties(${DFA_QT3D_TEST} PROPERTIES FOLDER "dfa/tools/viz")

else()
    #message(FATAL_ERROR "CGAL 3DCore not found")
    message(STATUS "CGAL 3DCore not found: disabling qt3d applications")
endif()
