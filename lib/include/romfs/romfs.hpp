#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#if __cplusplus > 202002L
#include <span>
#endif
#ifdef USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#include "nonstd/span.hpp"

#define ROMFS_CONCAT_IMPL(x, y) x##y
#define ROMFS_CONCAT(x, y) ROMFS_CONCAT_IMPL(x, y)

#define ROMFS_NAME ROMFS_CONCAT(RomFs_, LIBROMFS_PROJECT_NAME)

#if !defined(WIN32)
    #define ROMFS_VISIBILITY [[gnu::visibility("hidden")]]
#else
    #define ROMFS_VISIBILITY
#endif

namespace romfs {

    namespace impl {
        ROMFS_VISIBILITY void ROMFS_CONCAT(decompress_if_needed_, LIBROMFS_PROJECT_NAME)(std::vector<std::byte> &decompressedData, nonstd::span<const std::byte> compressedData);
    }

    class Resource {
    public:
        Resource() = default;
        explicit constexpr Resource(const nonstd::span<std::byte> &content) : m_compressedData(content) {}

        [[nodiscard]]
        const std::byte* data() const {
            impl::ROMFS_CONCAT(decompress_if_needed_, LIBROMFS_PROJECT_NAME)(m_decompressedData, m_compressedData);
            if (!m_decompressedData.empty())
                return this->m_decompressedData.data();
            else
                return this->m_compressedData.data();
        }

        [[nodiscard]]
        std::size_t size() const {
            impl::ROMFS_CONCAT(decompress_if_needed_, LIBROMFS_PROJECT_NAME)(m_decompressedData, m_compressedData);

            if (!this->m_decompressedData.empty())
                return this->m_decompressedData.size() - 1;
            else
                return m_compressedData.size_bytes() - 1;
        }

        [[nodiscard]]
        std::string_view string() const {
            return { reinterpret_cast<const char*>(this->data()), this->size() };
        }

        [[nodiscard]]
        bool valid() const {
            return !this->m_compressedData.empty() && this->m_compressedData.data() != nullptr;
        }

    private:
        mutable std::vector<std::byte> m_decompressedData;
        nonstd::span<const std::byte> m_compressedData;
    };

    namespace impl {

        struct ResourceLocation {
            std::string_view path;
            Resource resource;
        };

        [[nodiscard]] ROMFS_VISIBILITY const Resource& ROMFS_CONCAT(get_, LIBROMFS_PROJECT_NAME)(const fs::path &path);
        [[nodiscard]] ROMFS_VISIBILITY std::vector<fs::path> ROMFS_CONCAT(list_, LIBROMFS_PROJECT_NAME)(const fs::path &path);
        [[nodiscard]] ROMFS_VISIBILITY std::string_view ROMFS_CONCAT(name_, LIBROMFS_PROJECT_NAME)();

    }

    [[nodiscard]] ROMFS_VISIBILITY inline const Resource& get(const fs::path &path) { return impl::ROMFS_CONCAT(get_, LIBROMFS_PROJECT_NAME)(path); }
    [[nodiscard]] ROMFS_VISIBILITY inline std::vector<fs::path> list(const fs::path &path = {}) { return impl::ROMFS_CONCAT(list_, LIBROMFS_PROJECT_NAME)(path); }
    [[nodiscard]] ROMFS_VISIBILITY inline std::string_view name() { return impl::ROMFS_CONCAT(name_, LIBROMFS_PROJECT_NAME)(); }

}