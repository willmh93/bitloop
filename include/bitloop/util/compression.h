#pragma once
#include <string>
#include <string_view>

namespace Compression
{
    std::string b64_encode(std::string_view bytes);
    std::string b64_decode(std::string_view b64);

    std::string b64_encode_u64(std::uint64_t hash);
    std::uint64_t b64_decode_u64(std::string_view b64);

    bool valid_b62(std::string_view b62);

    // Brotli + ASCII-hex wrappers
    std::string brotli_ascii_compress(const std::string& input, int quality = 9, int window = 22);
    std::string brotli_ascii_decompress(const std::string& ascii);
}
