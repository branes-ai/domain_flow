# C++ header-only library cmake best practice

I have a C++ header-only library, what is the best cmake way to release this library so that another cmake project in an unrelated github repo can easily integrate it?

CMake for Header-Only Libraries
For a header-only C++ library, packaging it for CMake integration is relatively straightforward. Here's how to set it up so other CMake projects can easily use it:

Best Approach
 1. Create a CMake configuration that allows your library to be found by find_package()
 2. Set up proper installation targets that place headers in the expected locations
 3. Generate and export CMake config files to help consumers find and use your library


1. Project Structure
First, organize your library with a structure like:

```verbatim
your-library/
├── CMakeLists.txt
├── cmake/
│   └── your-library-config.cmake.in
├── include/
│   └── your-library/
│       ├── header1.hpp
│       ├── header2.hpp
│       └── ...
└── LICENSE, README.md, etc.
```

2. Main CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(your-library VERSION 1.0.0 LANGUAGES CXX)

# Create an interface library target (for header-only libraries)
add_library(your-library INTERFACE)

# Set include directories for users of your library
target_include_directories(your-library INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Set C++ standard if needed
target_compile_features(your-library INTERFACE cxx_std_11)  # adjust as needed

# Install the headers
install(DIRECTORY include/ DESTINATION include)

# Install the library target
install(TARGETS your-library
    EXPORT your-library-targets
    INCLUDES DESTINATION include
)

# Export targets
install(EXPORT your-library-targets
    FILE your-library-targets.cmake
    NAMESPACE your-library::
    DESTINATION lib/cmake/your-library
)

# Create and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/your-library-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/your-library-config.cmake
    INSTALL_DESTINATION lib/cmake/your-library
)

# Create and install version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/your-library-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/your-library-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/your-library-config-version.cmake
    DESTINATION lib/cmake/your-library
)
```

3. Config File Template

Create a simple config file in cmake/your-library-config.cmake.in:

```cmake
@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/your-library-targets.cmake")
check_required_components(your-library)
```

4. Make Your Library FetchContent-Compatible

Modern CMake projects often use FetchContent to pull dependencies directly from GitHub. Add this to your CMakeLists.txt:

```cmake
# Support for projects that want to use FetchContent
if(NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  # We're being included as a subproject
  add_library(your-library::your-library ALIAS your-library)
endif()
```

## How Other Projects Can Use Your Library

Once published, other projects can use your library in several ways:

### Method 1: Using find_package

```cmake
find_package(your-library REQUIRED)
target_link_libraries(your-target PRIVATE your-library::your-library)
```

### Method 2: Using FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
  your-library
  GIT_REPOSITORY https://github.com/your-username/your-library.git
  GIT_TAG v1.0.0  # tag, branch or commit hash
)
FetchContent_MakeAvailable(your-library)

target_link_libraries(your-target PRIVATE your-library::your-library)
```

### Method 3: Adding as a Git Submodule

```cmake
# In the main project
add_subdirectory(path/to/your-library)
target_link_libraries(your-target PRIVATE your-library::your-library)
```







