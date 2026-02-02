#include <fstream>
#include <string>
#include <string_view>
#include <vector>

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

namespace
{
    std::string replace(std::string string, const std::string &from, const std::string &to)
    {
        if (!from.empty())
            for (size_t pos = 0; (pos = string.find(from, pos)) != std::string::npos; pos += to.size())
                string.replace(pos, from.size(), to);
        return string;
    }

    std::string toPathString(std::string string)
    {
// Replace all backslashes with forward slashes on Windows
#if defined(_WIN32)
        string = replace(string, "\\", "/");
#endif

        string = replace(string, "\"", "\\\"");

        return string;
    }

    bool matchPattern(const std::string &path, const std::string &pattern)
    {
        if (pattern.empty())
            return false;

        std::string normalizedPath = path;
        std::string normalizedPattern = pattern;

#if defined(_WIN32)
        normalizedPath = replace(normalizedPath, "\\", "/");
        normalizedPattern = replace(normalizedPattern, "\\", "/");
#endif

        if (normalizedPath == normalizedPattern)
            return true;

        if (normalizedPattern.find("**") != std::string::npos)
        {
            size_t wildcardPos = normalizedPattern.find("**");
            std::string beforeWildcard = normalizedPattern.substr(0, wildcardPos);
            std::string afterWildcard = normalizedPattern.substr(wildcardPos + 2);

            if (!beforeWildcard.empty() && beforeWildcard.back() == '/')
            {
                beforeWildcard.pop_back();
            }
            if (!afterWildcard.empty() && afterWildcard.front() == '/')
            {
                afterWildcard = afterWildcard.substr(1);
            }

            bool startsMatch = beforeWildcard.empty() || normalizedPath.find(beforeWildcard) == 0;
            if (!startsMatch)
                return false;

            if (!afterWildcard.empty())
            {
                if (afterWildcard.find("**") != std::string::npos)
                {
                    size_t secondWildcard = afterWildcard.find("**");
                    std::string middlePart = afterWildcard.substr(0, secondWildcard);
                    if (!middlePart.empty() && middlePart.back() == '/')
                    {
                        middlePart.pop_back();
                    }

                    if (!middlePart.empty() && normalizedPath.find(middlePart) == std::string::npos)
                    {
                        return false;
                    }
                    return true;
                }

                if (afterWildcard.find('*') != std::string::npos)
                {
                    size_t starPos = afterWildcard.find('*');
                    std::string suffixBefore = afterWildcard.substr(0, starPos);
                    std::string suffixAfter = afterWildcard.substr(starPos + 1);

                    if (suffixBefore.empty() && !suffixAfter.empty())
                    {
                        return normalizedPath.size() >= suffixAfter.size() && normalizedPath.substr(normalizedPath.size() - suffixAfter.size()) == suffixAfter;
                    }
                }

                return normalizedPath.find(afterWildcard) != std::string::npos || (normalizedPath.size() >= afterWildcard.size() && normalizedPath.substr(normalizedPath.size() - afterWildcard.size()) == afterWildcard);
            }

            return true;
        }

        if (normalizedPattern.find('*') != std::string::npos)
        {
            size_t starPos = normalizedPattern.find('*');
            std::string before = normalizedPattern.substr(0, starPos);
            std::string after = normalizedPattern.substr(starPos + 1);

            if (normalizedPath.size() < before.size() + after.size())
                return false;

            bool startsMatch = normalizedPath.find(before) == 0;
            bool endsMatch = after.empty() || (normalizedPath.size() >= after.size() && normalizedPath.substr(normalizedPath.size() - after.size()) == after);

            return startsMatch && endsMatch;
        }

        return false;
    }

    bool shouldIncludeFile(const fs::path &filePath,
                           const std::vector<std::string> &includePatterns,
                           const std::vector<std::string> &excludePatterns)
    {
        std::string pathStr = filePath.string();

#if defined(_WIN32)
        pathStr = replace(pathStr, "\\", "/");
#endif

        if (!includePatterns.empty())
        {
            bool matchedInclude = false;
            for (const auto &pattern : includePatterns)
            {
                if (!pattern.empty() && matchPattern(pathStr, pattern))
                {
                    matchedInclude = true;
                    break;
                }
            }
            if (!matchedInclude)
                return false;
        }

        for (const auto &pattern : excludePatterns)
        {
            if (!pattern.empty() && matchPattern(pathStr, pattern))
            {
                return false;
            }
        }

        return true;
    }

    std::vector<std::string> parseIgnoreFile(const fs::path &resourcePath)
    {
        std::vector<std::string> patterns;
        fs::path ignoreFile = resourcePath / ".romfsignore";

        if (!fs::exists(ignoreFile))
            return patterns;

        std::ifstream file(ignoreFile.string());
        if (!file.is_open())
            return patterns;

        std::string line;
        while (std::getline(file, line))
        {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            size_t end = line.find_last_not_of(" \t\r\n");

            if (start == std::string::npos)
                continue; // Empty line

            line = line.substr(start, end - start + 1);

            // Skip comments and empty lines
            if (line.empty() || line[0] == '#')
                continue;

            patterns.push_back(line);
        }

        return patterns;
    }

}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::printf("Usage: ./libromfs-generator <PROJECT_NAME> <RESOURCE_LOCATION>\n");
        return 0;
    }
    std::ofstream outputFile("libromfs_resources.cpp");

    std::printf("[libromfs] Resource Folder: %s\n", argv[2]);

    std::vector<std::string> includePatterns; // Empty = include all
    std::vector<std::string> excludePatterns;

    // Read patterns from .romfsignore file
    excludePatterns = parseIgnoreFile(argv[2]);

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
    for (const auto &entry : fs::recursive_directory_iterator(argv[2]))
    {
        auto &p = entry.path();
        if (!fs::is_regular_file(p))
            continue;

        auto path = fs::canonical(fs::absolute(entry.path()));
        auto relativePath = fs::relative(entry.path(), fs::absolute(argv[2]));

        std::string filename = path.filename().string();
        if (filename == ".DS_Store" || filename == ".romfsignore")
        {
            std::printf("[libromfs] SKIP: %s\n", relativePath.string().c_str());
            continue;
        }

        if (!shouldIncludeFile(relativePath, includePatterns, excludePatterns))
        {
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

        if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
        {
            continue;
        }

        // Estimate the compressed size and allocate the buffer
        bytes.resize(inputData.size() * 1.1 + 12); // Slightly larger than the input size

        stream.avail_out = bytes.size();
        stream.next_out = bytes.data();

        // Perform the compression
        if (deflate(&stream, Z_FINISH) != Z_STREAM_END)
        {
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

        for (auto byte : bytes)
        {
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

        for (std::uint64_t i = 0; i < identifierCount; i++)
        {

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

        for (std::uint64_t i = 0; i < identifierCount; i++)
        {
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