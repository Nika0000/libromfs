project("libromfs-${LIBROMFS_PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 20)

# Gather romfs files
file(GLOB_RECURSE ROMFS_FILES
    "${LIBROMFS_RESOURCE_LOCATION}/*"
)

# Add sources
add_library(${PROJECT_NAME} STATIC
    ${ROMFS}
    source/romfs.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC include)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_compile_definitions(${PROJECT_NAME} PUBLIC LIBROMFS_PROJECT_NAME=${LIBROMFS_PROJECT_NAME})

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
        message(WARNING "Requested RomFS to be compressed but zlib is unavailable! Resulting RomFS will NOT be compressed.")
    endif()
endif()

# Make sure libromfs gets rebuilt when any of the resources are changed
if (LIBROMFS_PREBUILT_GENERATOR)
    message(STATUS "Using prebuilt libromfs-generator: ${LIBROMFS_PREBUILT_GENERATOR}")
    add_custom_command(OUTPUT ${ROMFS}
            COMMAND ${LIBROMFS_PREBUILT_GENERATOR}
                ${LIBROMFS_PROJECT_NAME} ${LIBROMFS_RESOURCE_LOCATION}
            DEPENDS ${ROMFS_FILES}
            )
else ()
    message(STATUS "Using libromfs-generator: $<TARGET_FILE:generator-${LIBROMFS_PROJECT_NAME}>")
    add_custom_command(OUTPUT ${ROMFS}
            COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:generator-${LIBROMFS_PROJECT_NAME}>
                ${LIBROMFS_PROJECT_NAME} ${LIBROMFS_RESOURCE_LOCATION}
            DEPENDS generator-${LIBROMFS_PROJECT_NAME} ${ROMFS_FILES}
            )
endif ()

add_custom_target(romfs_file_packer-${LIBROMFS_PROJECT_NAME} ALL DEPENDS ${ROMFS_FILES})