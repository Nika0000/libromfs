#include <fstream>
#include <string>
#include <vector>
#include <string_view>
#ifdef USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#if defined(LIBROMFS_COMPRESS_RESOURCES)
    #include <zlib.h>
#endif

namespace {

    std::string replace(std::string string, const std::string &from, const std::string &to) {
        if(!from.empty())
            for(size_t pos = 0; (pos = string.find(from, pos)) != std::string::npos; pos += to.size())
                string.replace(pos, from.size(), to);
        return string;
    }

    std::string toPathString(std::string string) {
        // Replace all backslashes with forward slashes on Windows
        #if defined (_WIN32)
            string = replace(string, "\\", "/");
        #endif

        // Replace all " with \"
        string = replace(string, "\"", "\\\"");

        return string;
    }

    std::vector<std::string> splitString(std::string string, const std::string &delimiter){
        std::vector<std::string> result;

        size_t position = 0;
        const size_t size = string.size();

        while (position < size) {
            position = string.find(delimiter);
            result.push_back(string.substr(0, position));
            string.erase(0, position + delimiter.size());
        }

        return result;
    }

    bool shouldExcludeFile(const fs::path &filePath, const std::vector<std::string> &excludeExtensions, const std::vector<std::string> &excludeFolders) {
        // Check if any folder in the path matches excluded folders
        for (const auto &part : filePath) {
            std::string partStr = part.string();
            for (const auto &excludeFolder : excludeFolders) {
                if (!excludeFolder.empty() && partStr == excludeFolder) {
                    return true;
                }
            }
        }

        // Check if file extension matches excluded extensions
        if (filePath.has_extension()) {
            std::string ext = filePath.extension().string();
            for (const auto &excludeExt : excludeExtensions) {
                if (!excludeExt.empty()) {
                    // Handle extensions with or without leading dot
                    std::string normalizedExclude = excludeExt;
                    if (!normalizedExclude.empty() && normalizedExclude[0] != '.') {
                        normalizedExclude = "." + normalizedExclude;
                    }

                    if (ext == normalizedExclude) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::printf("./libromfs-generator <LIBROMFS_PROJECT_NAME> <LIBROMFS_RESOURCE_LOCATION>");
        return 0;
    }
    std::ofstream outputFile("libromfs_resources.cpp");

    std::printf("[libromfs] Resource Folder: %s\n", argv[2]);

    std::vector<std::string> excludeExtensions;
    std::vector<std::string> excludeFolders;

#if defined(EXCLUDE_EXTENSIONS)
    excludeExtensions = splitString(EXCLUDE_EXTENSIONS, ",");
    std::printf("[libromfs] Excluding extensions: %s\n", EXCLUDE_EXTENSIONS);
#endif

#if defined(EXCLUDE_FOLDERS)
    excludeFolders = splitString(EXCLUDE_FOLDERS, ",");
    std::printf("[libromfs] Excluding folders: %s\n", EXCLUDE_FOLDERS);
#endif

    outputFile << "#include <romfs/romfs.hpp>\n\n";
    outputFile << "#include <array>\n";
    outputFile << "#include <cstdint>\n";

    outputFile << "\n\n";
    outputFile << "/* Compression flag - must match library compilation */\n";
    outputFile << "#ifndef LIBROMFS_COMPRESS_RESOURCES\n";
    outputFile << "    #define LIBROMFS_COMPRESS_RESOURCES 0\n";
    outputFile << "#endif\n";
    outputFile << "static_assert(LIBROMFS_COMPRESS_RESOURCES == ";
#if defined(LIBROMFS_COMPRESS_RESOURCES)
    outputFile << "1";
#else
    outputFile << "0";
#endif
    outputFile << ", \"Compression mismatch: generated resources and library must both be compiled with or without LIBROMFS_COMPRESS_RESOURCES\");\n";
    outputFile << "\n\n";
    outputFile << "/* Resource definitions */\n";

    std::vector<fs::path> paths;
    std::uint64_t identifierCount = 0;
    for (const auto &entry : fs::recursive_directory_iterator(argv[2])) {
        auto& p = entry.path();
        if (!fs::is_regular_file(p)) continue;

        auto path = fs::canonical(fs::absolute(entry.path()));
        auto relativePath = fs::relative(entry.path(), fs::absolute(argv[2]));

        if (path.filename().string() == ".DS_Store") {
            std::printf("[libromfs] SKIP: %s\n", relativePath.string().c_str());
            continue ;
        }

        // Check if this file should be excluded
        if (shouldExcludeFile(relativePath, excludeExtensions, excludeFolders)) {
            std::printf("[libromfs] Excluding: %s\n", relativePath.string().c_str());
            continue;
        }

        std::vector<std::uint8_t> inputData;
        inputData.resize(fs::file_size(p));

        auto file = std::fopen(entry.path().string().c_str(), "rb");
        inputData.resize(std::fread(inputData.data(), 1, fs::file_size(p), file));
        std::fclose(file);

        inputData.push_back(0x00);

        std::vector<std::uint8_t> bytes;
        #if defined(LIBROMFS_COMPRESS_RESOURCES)
            z_stream stream;
            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;
            stream.opaque = Z_NULL;
            stream.avail_in = inputData.size();
            stream.next_in = inputData.data();

            // Initialize the zlib deflate operation
            if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK) {
                continue;
            }

            // Estimate the compressed size and allocate the buffer
            bytes.resize(inputData.size() * 1.1 + 12); // Slightly larger than the input size

            stream.avail_out = bytes.size();
            stream.next_out  = bytes.data();

            // Perform the compression
            if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
                deflateEnd(&stream);
                continue;
            }

            // Resize the output buffer to the actual size
            bytes.resize(stream.total_out);

            // Clean up
            deflateEnd(&stream);
        #else
            bytes = std::move(inputData);
        #endif

        outputFile << "static std::array<std::uint8_t, " << bytes.size() << "> " << "resource_" + std::string(argv[1]) + "_" << identifierCount << " = {\n";
        outputFile << "    ";

        for (auto byte : bytes) {
            outputFile << static_cast<std::uint32_t>(byte) << ",";
        }

        outputFile << " };\n\n";

        paths.push_back(relativePath);

        identifierCount++;
    }

    outputFile << "\n";

    {
        outputFile << "/* Resource map */\n";
        outputFile << "ROMFS_VISIBILITY nonstd::span<romfs::impl::ResourceLocation> RomFs_" + std::string(argv[1]) + "_get_resources() {\n";
        outputFile << "    static std::array<romfs::impl::ResourceLocation, " << identifierCount << "> resources = {{\n";

        for (std::uint64_t i = 0; i < identifierCount; i++) {

            std::printf("[libromfs] Bundling resource: %s\n", paths[i].string().c_str());

            outputFile << "        " << "romfs::impl::ResourceLocation { \"" << toPathString(paths[i].string()) << "\", romfs::Resource({ reinterpret_cast<std::byte*>(resource_" + std::string(argv[1]) + "_" << i << ".data()), " << "resource_" + std::string(argv[1]) + "_" << i << ".size() }) " << "},\n";
        }
        outputFile << "    }};";

        outputFile << "\n\n    return resources;\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";

    {
        outputFile << "/* Resource paths */\n";
        outputFile << "ROMFS_VISIBILITY nonstd::span<std::string_view> RomFs_" + std::string(argv[1]) + "_get_paths() {\n";
        outputFile << "    static std::array<std::string_view, " << identifierCount << "> paths = {{\n";

        for (std::uint64_t i = 0; i < identifierCount; i++) {
            outputFile << "        \"" << toPathString(paths[i].string()) << "\",\n";
        }
        outputFile << "    }};";

        outputFile << "\n\n    return paths;\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";

    {
        outputFile << "/* RomFS name */\n";
        outputFile << "ROMFS_VISIBILITY const char* RomFs_" + std::string(argv[1]) + "_get_name() {\n";
        outputFile << "    return \"" + std::string(argv[1]) + "\";\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";
}