#pragma once
#include <string>
#include <string_view>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <span>

#include <brotli/encode.h>
#include <brotli/decode.h>

namespace compression
{
    struct BrotliDict
    {
        std::vector<uint8_t> bytes;
        BrotliEncoderPreparedDictionary* prepared = nullptr;

        BrotliDict() = default;
        BrotliDict(std::span<const std::string_view> dict_tokens)
        {
            build(dict_tokens);
        }
        BrotliDict(std::initializer_list<std::string_view> parts)
        {
            build(std::span<const std::string_view>(parts.begin(), parts.size()));
        }

        void build(std::span<const std::string_view> parts);

        BrotliDict(const BrotliDict&) = delete;
        BrotliDict& operator=(const BrotliDict&) = delete;

        BrotliDict(BrotliDict&& other) noexcept
            : bytes(std::move(other.bytes)), prepared(other.prepared)
        {
            other.prepared = nullptr;
        }

        BrotliDict& operator=(BrotliDict&& other) noexcept;
        ~BrotliDict();

        bool empty() const { return prepared == nullptr || bytes.empty(); }
    };

    std::string b62_encode(std::string_view bytes);
    std::string b62_decode(std::string_view s);

    std::string b64_encode(std::string_view bytes);
    std::string b64_decode(std::string_view b64);

    std::string b64_encode_u64(std::uint64_t hash);
    std::uint64_t b64_decode_u64(std::string_view b64);

    bool valid_b62(std::string_view b62);

    // Brotli + ASCII-hex wrappers
    std::string brotli_ascii_compress(const std::string& input, int quality = 9, int window = 22);
    std::string brotli_ascii_decompress(const std::string& ascii);

    std::string brotli_ascii_compress_with_dict(const std::string& input, int quality, int window, const BrotliDict* dict);
    std::string brotli_ascii_decompress_with_dict(const std::string& ascii, const BrotliDict* dict);
}
