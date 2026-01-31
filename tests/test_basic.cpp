#include "test_framework.hpp"
#include <romfs/romfs.hpp>
#include <iostream>
#include <cstring>
#include <cassert>

using namespace test;

// Test: Get project name
TEST(project_name) {
    auto name = romfs::name();
    ASSERT(!name.empty(), "Project name should not be empty");
    ASSERT_STR_EQ(name, "test_project", "Project name should be 'test_project'");
}

// Test: Get existing file
TEST(get_existing_file) {
    auto resource = romfs::get("hello.txt");
    ASSERT(resource.valid(), "Resource should be valid");
    ASSERT(resource.size() > 0, "Resource size should be greater than 0");
}

// Test: Get file content
TEST(get_file_content) {
    auto resource = romfs::get("hello.txt");
    auto content = resource.string();
    ASSERT_STR_EQ(content, "Hello, libromfs!", "File content should match");
}

// Test: Get file size
TEST(get_file_size) {
    auto resource = romfs::get("hello.txt");
    ASSERT_EQ(resource.size(), 16, "File size should be 16 bytes");
}

// Test: Get file data pointer
TEST(get_file_data_pointer) {
    auto resource = romfs::get("hello.txt");
    const std::byte* data = resource.data();
    ASSERT(data != nullptr, "Data pointer should not be null");

    // Check first few bytes
    ASSERT(static_cast<char>(data[0]) == 'H', "First byte should be 'H'");
    ASSERT(static_cast<char>(data[1]) == 'e', "Second byte should be 'e'");
}

// Test: Get non-existent file throws exception
TEST(get_nonexistent_file_throws) {
    bool threw = false;
    try {
        auto resource = romfs::get("does_not_exist.txt");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    ASSERT(threw, "Getting non-existent file should throw std::invalid_argument");
}

// Test: List all files
TEST(list_all_files) {
    auto files = romfs::list();
    ASSERT(files.size() > 0, "Should have at least one file");

    bool found_hello = false;
    bool found_json = false;
    bool found_binary = false;
    bool found_nested = false;

    for (const auto& file : files) {
        std::string path = file.string();
        if (path.find("hello.txt") != std::string::npos) found_hello = true;
        if (path.find("data.json") != std::string::npos) found_json = true;
        if (path.find("binary.bin") != std::string::npos) found_binary = true;
        if (path.find("nested.txt") != std::string::npos) found_nested = true;
    }

    ASSERT(found_hello, "Should find hello.txt");
    ASSERT(found_json, "Should find data.json");
    ASSERT(found_binary, "Should find binary.bin");
    ASSERT(found_nested, "Should find nested.txt");
}

// Test: List files in subdirectory
TEST(list_subdirectory) {
    auto files = romfs::list("subdir");
    ASSERT(files.size() > 0, "Subdirectory should have at least one file");

    bool found_nested = false;
    for (const auto& file : files) {
        if (file.filename() == "nested.txt") {
            found_nested = true;
        }
    }
    ASSERT(found_nested, "Should find nested.txt in subdir");
}

// Test: Get nested file
TEST(get_nested_file) {
    auto resource = romfs::get("subdir/nested.txt");
    ASSERT(resource.valid(), "Nested resource should be valid");
    auto content = resource.string();
    ASSERT(content.find("subdirectory") != std::string::npos, "Content should mention 'subdirectory'");
}

// Test: Binary file integrity
TEST(binary_file_integrity) {
    auto resource = romfs::get("binary.bin");
    ASSERT(resource.valid(), "Binary resource should be valid");
    ASSERT(resource.size() > 0, "Binary file should have size > 0");

    // Check PNG signature
    const std::byte* data = resource.data();
    ASSERT(static_cast<unsigned char>(data[0]) == 0x89, "PNG signature byte 0 should be 0x89");
    ASSERT(static_cast<unsigned char>(data[1]) == 0x50, "PNG signature byte 1 should be 0x50 'P'");
    ASSERT(static_cast<unsigned char>(data[2]) == 0x4E, "PNG signature byte 2 should be 0x4E 'N'");
    ASSERT(static_cast<unsigned char>(data[3]) == 0x47, "PNG signature byte 3 should be 0x47 'G'");
}

// Test: JSON file content
TEST(json_file_content) {
    auto resource = romfs::get("data.json");
    auto content = resource.string();
    ASSERT(content.find("\"name\"") != std::string::npos, "JSON should contain 'name' field");
    ASSERT(content.find("libromfs") != std::string::npos, "JSON should contain 'libromfs' value");
    ASSERT(content.find("compression") != std::string::npos, "JSON should mention 'compression'");
}
