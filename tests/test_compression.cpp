#include "test_framework.hpp"
#include <romfs/romfs.hpp>
#include <iostream>
#include <cstring>
#include <vector>

using namespace test;

// Only run compression tests if compression is enabled
#ifdef LIBROMFS_COMPRESS_RESOURCES

// Test: Compressed file still returns correct size
TEST(compressed_file_size) {
    auto resource = romfs::get("hello.txt");
    ASSERT_EQ(resource.size(), 16, "Compressed file should still report correct uncompressed size");
}

// Test: Compressed file content is intact
TEST(compressed_file_content) {
    auto resource = romfs::get("hello.txt");
    auto content = resource.string();
    ASSERT(content == "Hello, libromfs!", "Compressed file content should match original");
}

// Test: Compressed binary file integrity
TEST(compressed_binary_integrity) {
    auto resource = romfs::get("binary.bin");
    ASSERT(resource.valid(), "Compressed binary resource should be valid");
    
    // Check PNG signature is preserved after compression/decompression
    const std::byte* data = resource.data();
    ASSERT(static_cast<unsigned char>(data[0]) == 0x89, "PNG signature byte 0 should be preserved");
    ASSERT(static_cast<unsigned char>(data[1]) == 0x50, "PNG signature byte 1 should be preserved");
    ASSERT(static_cast<unsigned char>(data[2]) == 0x4E, "PNG signature byte 2 should be preserved");
    ASSERT(static_cast<unsigned char>(data[3]) == 0x47, "PNG signature byte 3 should be preserved");
}

// Test: Multiple accesses to same compressed file (caching)
TEST(compressed_file_caching) {
    auto resource = romfs::get("data.json");
    
    // First access - triggers decompression
    auto content1 = resource.string();
    auto size1 = resource.size();
    auto data1 = resource.data();
    
    // Second access - should use cached data
    auto content2 = resource.string();
    auto size2 = resource.size();
    auto data2 = resource.data();
    
    ASSERT(content1 == content2, "Content should be identical on multiple accesses");
    ASSERT_EQ(size1, size2, "Size should be identical on multiple accesses");
    ASSERT(data1 == data2, "Data pointer should be identical (cached)");
}

// Test: Compressed nested file
TEST(compressed_nested_file) {
    auto resource = romfs::get("subdir/nested.txt");
    auto content = resource.string();
    ASSERT(content.find("subdirectory") != std::string::npos, 
           "Compressed nested file content should be correct");
}

// Test: List works with compressed files
TEST(list_compressed_files) {
    auto files = romfs::list();
    ASSERT(files.size() >= 4, "Should still list all files when compressed");
}

// Test: Large file compression (JSON file)
TEST(compressed_json_file) {
    auto resource = romfs::get("data.json");
    auto content = resource.string();
    
    // Verify JSON structure is preserved
    ASSERT(content.find("{") != std::string::npos, "JSON should start with '{'");
    ASSERT(content.find("}") != std::string::npos, "JSON should end with '}'");
    ASSERT(content.find("\"features\"") != std::string::npos, "JSON features field should be present");
}

#endif // LIBROMFS_COMPRESS_RESOURCES
