#pragma once
#include <asset/import.h>
#include <asset/resource.h>

#include <bimg/bimg.h>
#include <bimg/encode.h>
#include <bimg/decode.h>
#include <bx/allocator.h>

#include <memory>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace roj {
    namespace {

        bx::AllocatorI* imageAllocator() {
            static bx::DefaultAllocator allocator;
            return &allocator;
        }
        std::optional<std::shared_ptr<bimg::ImageContainer>> withMips(bimg::ImageContainer* decoded) {
            const std::shared_ptr<bimg::ImageContainer> base(decoded, bimg::imageFree); // freed once the mipped copy exists
            if (!base)
                return std::nullopt;
            std::shared_ptr<bimg::ImageContainer> mipped(bimg::imageGenerateMips(imageAllocator(), *base), bimg::imageFree);
            if (!mipped)
                return std::nullopt;
            return mipped;
        }


        std::optional<std::shared_ptr<bimg::ImageContainer>> decodeImage(const void* bytes, size_t size) {
            if (size == 0)
                return std::nullopt;
            return withMips(bimg::imageParse(imageAllocator(), bytes, static_cast<uint32_t>(size), bimg::TextureFormat::RGBA8));
        }

        std::optional<std::shared_ptr<bimg::ImageContainer>> loadTextureFile(const std::filesystem::path& file) {
            std::ifstream stream(file, std::ios::binary);
            const std::vector<char> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
            return decodeImage(bytes.data(), bytes.size());
        }

        std::optional<std::shared_ptr<bimg::ImageContainer>> decodeCompressedTexture(const EmbeddedTexture& embedded) {
            return decodeImage(embedded.data.data(), uint32_t(embedded.data.size()));
        }

        std::optional<std::shared_ptr<bimg::ImageContainer>> copyRawTexture(const EmbeddedTexture& embedded) {
            if (embedded.data.empty() || embedded.height == 0 || embedded.width == 0)
                return std::nullopt;

            constexpr size_t kRgbaChannels = 4;

            const size_t texelCount =
                static_cast<size_t>(embedded.width) * embedded.height;

            if (texelCount > SIZE_MAX / kRgbaChannels ||
                embedded.data.size() < texelCount * kRgbaChannels)
                return std::nullopt;

            bimg::ImageContainer* raw = bimg::imageAlloc(
                imageAllocator(),
                bimg::TextureFormat::RGBA8,
                embedded.width,
                embedded.height,
                1,
                1,
                false,
                false);

            if (!raw)
                return std::nullopt;

            auto* texels = static_cast<uint8_t*>(raw->m_data);
            const uint8_t* source = embedded.data.data();

            // Assimp raw texels are stored in B, G, R, A order.
            for (size_t t = 0; t < texelCount; ++t) {
                const size_t offset = t * kRgbaChannels;

                texels[offset + 0] = source[offset + 2]; // R
                texels[offset + 1] = source[offset + 1]; // G
                texels[offset + 2] = source[offset + 0]; // B
                texels[offset + 3] = source[offset + 3]; // A
            }

            return withMips(raw);
        }

        std::optional<std::shared_ptr<bimg::ImageContainer>> readEmbeddedTexture(const EmbeddedTexture& embedded) {
            if (embedded.height == 0)
                return decodeCompressedTexture(embedded);
            return copyRawTexture(embedded);
        }
    }
} // namespace

