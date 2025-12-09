cmake_minimum_required(VERSION 3.16)
project(FPrime C CXX)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the cmake script directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Forbid in-source builds
include(NoInSourceBuilds)

# Include testing configuration (googletest)
include(Testing)

# Set output directories for binaries and libraries
include(SetOutputDirectories)

# Set default warning flags for all builds
# Specific build modules that do not comply with these flags can disable one or more of them
#
# -Wno-unused-parameter: Disable the unused parameter warning for now. F' has a lot of interfaces,
# so unused method parameters are common in the F prime code base. Eventually all intentionally
# unused parameters should be annotated to avoid this error.
#
# -Wno-vla: Variable length arrays are required to support sending to async serializable
# ports. https://github.com/nasa/fprime/issues/945
#
add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
    -pedantic
    -Wall
    -Wextra
    -Wconversion
    -Wdouble-promotion
    -Wshadow
    -Werror
    -Wno-unused-parameter
    -Wno-vla
)

# Add the src directory to the project
add_subdirectory(src)
