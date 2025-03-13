# Package management

A common issue when using `find_package()` with libraries installed via package managers like vcpkg is
how to configure the CMake build to find the installation. 

**Understanding the Problem**

* **vcpkg's CMake Integration:** vcpkg does provide CMake integration, but it doesn't automatically modify your system-wide CMake paths. Instead, it creates a toolchain file that tells CMake where to find the installed libraries.
* **`find_package()`'s Search Locations:** `find_package()` searches in specific locations, and without proper guidance, it won't find CGAL in vcpkg's installation directory.

**The Solution: Using the vcpkg Toolchain File**

1.  **Locate the vcpkg Toolchain File:**
    * The toolchain file is usually located in the vcpkg installation directory, within the `scripts/buildsystems/vcpkg.cmake` path.
    * For example, if you installed vcpkg in `C:\vcpkg`, the toolchain file would be at `C:\vcpkg\scripts\buildsystems\vcpkg.cmake`.
    * If you installed vcpkg in your home directory within linux, it might be located at `~/vcpkg/scripts/buildsystems/vcpkg.cmake`

2.  **Tell CMake to Use the Toolchain File:**
    * You can do this in several ways:
        * **Command-Line Option:** When configuring your CMake project, use the `-DCMAKE_TOOLCHAIN_FILE` option:

        ```bash
        cmake -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg.cmake> <path_to_your_source_directory>
        ```

        * Replace `<path_to_vcpkg.cmake>` with the actual path to the `vcpkg.cmake` file.
        * Replace `<path_to_your_source_directory>` with the directory that contains your CMakeLists.txt file.
        * **CMake Preset:** If you are using CMake presets, you can add the toolchain file to your preset configuration. This is the modern and recomended way.
        * **Environment Variable (Less Recommended):** You can set the `CMAKE_TOOLCHAIN_FILE` environment variable, but this is generally less flexible than the command-line option.

3.  **CMakeLists.txt:**
    * Your `CMakeLists.txt` file should then use `find_package(CGAL REQUIRED)` as you normally would:

        ```cmake
        cmake_minimum_required(VERSION 3.10)
        project(MyCGALProject)

        find_package(CGAL REQUIRED)

        if(CGAL_FOUND)
            include_directories(${CGAL_INCLUDE_DIRS})
            add_executable(my_executable main.cpp)
            target_link_libraries(my_executable ${CGAL_LIBRARIES})
        else()
            message(FATAL_ERROR "CGAL not found")
        endif()
        ```

**Example Workflow (Command-Line)**

1.  **Create a simple project:**
    * Create a directory for your project.
    * Inside, create `main.cpp` and `CMakeLists.txt`.

2.  **`main.cpp` (Example):**

    ```cpp
    #include <iostream>
    #include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

    int main() {
        std::cout << "CGAL is working!" << std::endl;
        return 0;
    }
    ```

3.  **`CMakeLists.txt`:**

    ```cmake
    cmake_minimum_required(VERSION 3.10)
    project(MyCGALProject)

    find_package(CGAL REQUIRED)

    if(CGAL_FOUND)
        include_directories(${CGAL_INCLUDE_DIRS})
        add_executable(my_executable main.cpp)
        target_link_libraries(my_executable ${CGAL_LIBRARIES})
    else()
        message(FATAL_ERROR "CGAL not found")
    endif()
    ```

4.  **Configure and Build:**

    ```bash
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg.cmake> ..
    cmake --build .
    ./my_executable
    ```

**Important Notes**

* **Correct vcpkg Installation:** Ensure that CGAL is correctly installed in your vcpkg instance. You can verify this using `vcpkg list cgal`.
* **32/64 bit consistency:** ensure that your vcpkg triplet, and your compiler triplet are the same. Mixing 32 and 64 bit libraries will cause linking errors.
* **Triplets:** vcpkg uses triplets to specify the target architecture and compiler (e.g., `x64-windows`, `x64-linux`). Make sure you're using the correct triplet when installing CGAL and when configuring your CMake project. If you are unsure, you can specify the triplet with the cmake command, for instance: `-DVCPKG_TARGET_TRIPLET=x64-windows`.
* **CMake Presets:** CMake presets are a more modern and maintainable way to configure your builds. They allow you to store your build settings in a file (e.g., `CMakePresets.json`) and easily switch between different configurations.

By using the vcpkg toolchain file, you tell CMake where to find the libraries installed by vcpkg, allowing `find_package()` to work correctly.

