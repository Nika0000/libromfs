cmake_minimum_required(VERSION 3.16)
project(libromfs-tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Test project name
set(LIBROMFS_PROJECT_NAME "test_project")
set(LIBROMFS_RESOURCE_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/resources")

# Add libromfs
add_subdirectory(.. libromfs)

# Create test executable
add_executable(libromfs-test
    test_main.cpp
    test_basic.cpp
    test_compression.cpp
)

target_link_libraries(libromfs-test PRIVATE ${LIBROMFS_LIBRARY})
target_include_directories(libromfs-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Enable testing
enable_testing()
add_test(NAME libromfs-test COMMAND libromfs-test)
