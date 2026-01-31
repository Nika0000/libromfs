# Check if this is being built standalone or as part of parent project
if(NOT DEFINED LIBROMFS_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.16)
    set(LIBROMFS_PROJECT_NAME "standalone" CACHE STRING "Project name for standalone build")
endif()

project(generator-${LIBROMFS_PROJECT_NAME})
set(CMAKE_CXX_STANDARD 20)

# This is a code generator, so when cross compiling it should be built with the host (native) toolchain
if (CMAKE_CROSSCOMPILING)
    SET(CMAKE_C_COMPILER ${NATIVE_CMAKE_C_COMPILER})
    SET(CMAKE_CXX_COMPILER ${NATIVE_CMAKE_CXX_COMPILER})
    unset(CMAKE_C_OSX_DEPLOYMENT_TARGET_FLAG)
    unset(CMAKE_CXX_OSX_DEPLOYMENT_TARGET_FLAG)
    SET(CMAKE_C_FLAGS ${NATIVE_CMAKE_C_FLAGS})
    SET(CMAKE_CXX_FLAGS ${NATIVE_CMAKE_CXX_FLAGS})
endif()

# Add sources
add_executable(${PROJECT_NAME}
    source/main.cpp
)

# Pass project name and resource location as compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE LIBROMFS_PROJECT_NAME="${LIBROMFS_PROJECT_NAME}")

# Pass exclusion options if defined
if (LIBROMFS_EXCLUDE_EXTENSIONS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE EXCLUDE_EXTENSIONS="${LIBROMFS_EXCLUDE_EXTENSIONS}")
endif()

if (LIBROMFS_EXCLUDE_FOLDERS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE EXCLUDE_FOLDERS="${LIBROMFS_EXCLUDE_FOLDERS}")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE include)

if (USE_BOOST_FILESYSTEM)
    find_package(Boost 1.44 REQUIRED COMPONENTS filesystem)
    if(Boost_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE Boost::filesystem)
        target_compile_options(${PROJECT_NAME} PRIVATE -DUSE_BOOST_FILESYSTEM)
    endif()
endif ()

if (LIBROMFS_COMPRESS_RESOURCES)
    find_package(ZLIB QUIET)
    if (ZLIB_FOUND)
        list(LENGTH ZLIB_LIBRARIES ZLIB_LIBRARIES_COUNT)

        if (${ZLIB_LIBRARIES_COUNT} EQUAL 1)
            if (TARGET ${ZLIB_LIBRARIES})
                target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES})
                target_compile_definitions(${PROJECT_NAME} PRIVATE LIBROMFS_COMPRESS_RESOURCES=1)
            endif()
        elseif(${ZLIB_LIBRARIES_COUNT} GREATER 1)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES})
            target_include_directories(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
            target_compile_definitions(${PROJECT_NAME} PRIVATE LIBROMFS_COMPRESS_RESOURCES=1)
        endif()
    else()
        message(WARNING "Requested RomFS generator to be built with compression but zlib is unavailable! Resources will NOT be compressed.")
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE "/EHsc")
endif()

# Export generated romfs resource file to libromfs
set(ROMFS "libromfs_resources.cpp" PARENT_SCOPE)
set(ROMFS "libromfs_resources.cpp")