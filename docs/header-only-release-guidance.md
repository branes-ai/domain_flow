# Releasing a C++ header-only library

Releasing a header-only C++ library for easy integration into other CMake projects requires careful consideration of packaging, installation, and dependency management. Here's a breakdown of the best practices using CMake:

**1. Project Structure:**

* **`include/`:** Place your header files within an `include` directory. This is standard practice and makes it easy for users to find the necessary headers.
* **`CMakeLists.txt`:** A `CMakeLists.txt` file at the root of your project.
* **`README.md`:** A clear `README.md` explaining how to use your library.
* **Optional:** `examples/`, `tests/`, etc.

**2. `CMakeLists.txt` Configuration:**

```cmake
cmake_minimum_required(VERSION 3.15) # Or a suitable minimum version

project(MyHeaderOnlyLibrary VERSION 1.0.0)

# Install the headers
install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h"
        PATTERN "*.hpp")

# Generate a CMake config file for find_package
include(CMakePackageConfigHelpers)

# Configure the package
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/MyHeaderOnlyLibraryConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/MyHeaderOnlyLibraryConfig.cmake"
  INSTALL_DESTINATION "lib/cmake/MyHeaderOnlyLibrary"
  VERSION_VAR MyHeaderOnlyLibrary_VERSION
)

# Install the config file
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MyHeaderOnlyLibraryConfig.cmake"
        DESTINATION "lib/cmake/MyHeaderOnlyLibrary")

# Install the config version file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/MyHeaderOnlyLibraryConfigVersion.cmake"
  VERSION ${MyHeaderOnlyLibrary_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MyHeaderOnlyLibraryConfigVersion.cmake"
        DESTINATION "lib/cmake/MyHeaderOnlyLibrary")

# Export targets (though in header-only, it's about the include path)
export(PACKAGE MyHeaderOnlyLibrary)

install(EXPORT MyHeaderOnlyLibraryTargets
        FILE MyHeaderOnlyLibraryTargets.cmake
        NAMESPACE MyHeaderOnlyLibrary::
        DESTINATION lib/cmake/MyHeaderOnlyLibrary)

# Generate the myHeaderOnlyLibraryTargets.cmake file.
export_targets(MyHeaderOnlyLibraryTargets FILE "${CMAKE_CURRENT_BINARY_DIR}/MyHeaderOnlyLibraryTargets.cmake")
```

**3. `MyHeaderOnlyLibraryConfig.cmake.in`:**

This file is used to generate the `MyHeaderOnlyLibraryConfig.cmake` file.

```cmake
@PACKAGE_INIT@

set_property(TARGET MyHeaderOnlyLibrary::MyHeaderOnlyLibrary INTERFACE PROPERTY INCLUDE_DIRECTORIES "@PACKAGE_INCLUDE_DIRS@")

# Example if you have any dependencies. Otherwise do not put any find_package commands here.
# find_package(SomeDependency REQUIRED)
# if(TARGET SomeDependency::SomeDependency)
#   set_property(TARGET MyHeaderOnlyLibrary::MyHeaderOnlyLibrary INTERFACE PROPERTY INTERFACE_LINK_LIBRARIES SomeDependency::SomeDependency)
# endif()

# Example if you have any compile definitions.
# set_property(TARGET MyHeaderOnlyLibrary::MyHeaderOnlyLibrary INTERFACE PROPERTY INTERFACE_COMPILE_DEFINITIONS MyHeaderOnlyLibrary_COMPILE_DEFINITIONS)
```

**4. Installation:**

When a user installs your library (e.g., using `cmake --install .`), the `include` directory and the CMake configuration files will be placed in the specified installation directories.

**5. User Integration:**

In the user's CMake project:

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyUserProject)

# Find your header-only library
find_package(MyHeaderOnlyLibrary REQUIRED)

# Include the headers
include_directories(${MyHeaderOnlyLibrary_INCLUDE_DIRS})

# Example usage
add_executable(my_executable main.cpp)
target_link_libraries(my_executable MyHeaderOnlyLibrary::MyHeaderOnlyLibrary)
```

**6. Packaging and Distribution:**

* **GitHub Releases:** Create tagged releases on GitHub. Users can download the source code and build/install it themselves.
* **CMake FetchContent:** Users can use `FetchContent` to download and build your library directly from GitHub. This is convenient for users but requires them to have internet access during the build.
* **Package Managers (Conan, vcpkg):** Consider making your library available through package managers like Conan or vcpkg. This simplifies dependency management for users.

**Using FetchContent for Integration:**

```cmake
include(FetchContent)

FetchContent_Declare(
  MyHeaderOnlyLibrary
  GIT_REPOSITORY https://github.com/yourusername/MyHeaderOnlyLibrary.git
  GIT_TAG        v1.0.0 # Or a specific tag/commit
)

FetchContent_MakeAvailable(MyHeaderOnlyLibrary)

include_directories(${MyHeaderOnlyLibrary_SOURCE_DIR}/include)

add_executable(my_executable main.cpp)
target_link_libraries(my_executable MyHeaderOnlyLibrary::MyHeaderOnlyLibrary)
```

**Key Improvements and Explanations:**

* **`CMakePackageConfigHelpers`:** This module generates the `MyHeaderOnlyLibraryConfig.cmake` file, which is crucial for `find_package`.
* **`MyHeaderOnlyLibraryConfig.cmake.in`:** This template file allows you to configure the library's include directories and other properties.
* **`write_basic_package_version_file`:** This creates a version file that `find_package` uses to verify compatibility.
* **`export_targets`:** This creates a targets file that allows the user to link to the library using `target_link_libraries(my_executable MyHeaderOnlyLibrary::MyHeaderOnlyLibrary)`. Even though it is header-only, this allows for the include directories to be added, and other properties to be added.
* **`FetchContent`:** Provides a clean way for users to integrate your library directly from GitHub.
* **`include_directories(${MyHeaderOnlyLibrary_INCLUDE_DIRS})`:** This is crucial for adding the include directories from your library to the user's project.
* **`target_link_libraries(my_executable MyHeaderOnlyLibrary::MyHeaderOnlyLibrary)`:** Even though it is a header only library, this helps to add the include directories, and other properties that are set in the config.

By following these steps, you can create a robust and user-friendly release for your header-only C++ library.
