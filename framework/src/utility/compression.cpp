#include <bitloop/utility/compression.h>

#include <brotli/encode.h>
#include <brotli/decode.h>
#include <stdexcept>
#include <vector>
#include <cctype>

namespace
{
    //constexpr char lookup[] =
    //    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    //    "abcdefghijklmnopqrstuvwxyz"
    //    "0123456789+/";

    constexpr char lookup[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";


    //inline unsigned char b64_value(unsigned char c)
    //{
    //    if (c >= 'A' && c <= 'Z') return static_cast<unsigned char>(c - 'A');
    //    if (c >= 'a' && c <= 'z') return static_cast<unsigned char>(26 + (c - 'a'));
    //    if (c >= '0' && c <= '9') return static_cast<unsigned char>(52 + (c - '0'));
    //    if (c == '+') return 62;
    //    if (c == '/') return 63;
    //    return 0xFF; // invalid
    //}

    inline unsigned char b64_value(unsigned char c)
    {
        if (c >= 'A' && c <= 'Z') return static_cast<unsigned char>(c - 'A');
        if (c >= 'a' && c <= 'z') return static_cast<unsigned char>(26 + (c - 'a'));
        if (c >= '0' && c <= '9') return static_cast<unsigned char>(52 + (c - '0'));
        if (c == '-') return 62;
        if (c == '_') return 63;
        return 0xFF; // invalid
    }
}


namespace Compression {

    // ========== Base64 encode ==========
    std::string b64_encode(std::string_view bytes)
    {
        const unsigned char* src = reinterpret_cast<const unsigned char*>(bytes.data());
        size_t len = bytes.size();
        if (len == 0) return {};

        size_t out_len = ((len + 2) / 3) * 4;
        std::string out(out_len, '=');

        size_t si = 0, oi = 0;
        while (si + 3 <= len) {
            uint32_t v = (uint32_t(src[si]) << 16) | (uint32_t(src[si + 1]) << 8) | uint32_t(src[si + 2]);
            out[oi++] = lookup[(v >> 18) & 63];
            out[oi++] = lookup[(v >> 12) & 63];
            out[oi++] = lookup[(v >> 6) & 63];
            out[oi++] = lookup[v & 63];
            si += 3;
        }

        size_t rem = len - si;
        if (rem == 1) {
            uint32_t v = uint32_t(src[si]) << 16;
            out[oi++] = lookup[(v >> 18) & 63];
            out[oi++] = lookup[(v >> 12) & 63];
            // '=' padding already present in out
        }
        else if (rem == 2) {
            uint32_t v = (uint32_t(src[si]) << 16) | (uint32_t(src[si + 1]) << 8);
            out[oi++] = lookup[(v >> 18) & 63];
            out[oi++] = lookup[(v >> 12) & 63];
            out[oi++] = lookup[(v >> 6) & 63];
            // last '=' already present
        }
        return out;
    }

    // ========== Base64 decode ==========
    std::string b64_decode(std::string_view b64)
    {
        // Remove ASCII whitespace
        std::string clean;
        clean.reserve(b64.size());
        for (unsigned char c : std::string(b64)) {
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
            clean.push_back(char(c));
        }
        if (clean.empty()) return {};

        // Auto-pad to multiple of 4
        while (clean.size() % 4 != 0) clean.push_back('=');

        // Compute padding count
        size_t pad = 0;
        if (!clean.empty() && clean.back() == '=') {
            pad++;
            if (clean.size() >= 2 && clean[clean.size() - 2] == '=')
                pad++;
        }

        // Allocate output
        size_t out_len = (clean.size() / 4) * 3 - pad;
        std::string out(out_len, '\0');

        size_t oi = 0;
        for (size_t i = 0; i < clean.size(); i += 4)
        {
            unsigned char c0 = clean[i + 0];
            unsigned char c1 = clean[i + 1];
            unsigned char c2 = clean[i + 2];
            unsigned char c3 = clean[i + 3];

            if (c0 == '=' || c1 == '=') throw std::runtime_error("base64 decode: invalid early padding");

            unsigned char v0 = b64_value(c0);
            unsigned char v1 = b64_value(c1);
            if (v0 == 0xFF || v1 == 0xFF) throw std::runtime_error("base64 decode: invalid character");

            unsigned char v2 = (c2 == '=') ? 0 : b64_value(c2);
            unsigned char v3 = (c3 == '=') ? 0 : b64_value(c3);
            if ((c2 != '=' && v2 == 0xFF) || (c3 != '=' && v3 == 0xFF))
                throw std::runtime_error("base64 decode: invalid character");

            uint32_t v = (uint32_t(v0) << 18) | (uint32_t(v1) << 12) | (uint32_t(v2) << 6) | uint32_t(v3);

            out[oi++] = char((v >> 16) & 0xFF);
            if (c2 != '=') {
                if (oi >= out_len) break;
                out[oi++] = char((v >> 8) & 0xFF);
            }
            if (c3 != '=') {
                if (oi >= out_len) break;
                out[oi++] = char(v & 0xFF);
            }
        }
        return out;
    }

    // ========== Brotli + Base64 ==========
    std::string brotli_b64_compress(std::string_view input, int quality, int window)
    {
        const uint8_t* in = reinterpret_cast<const uint8_t*>(input.data());
        size_t in_len = input.size();

        size_t cap = BrotliEncoderMaxCompressedSize(in_len);
        if (cap == 0) cap = in_len + 512; // tiny inputs
        std::string buf(cap, '\0');
        size_t out_size = cap;

        BROTLI_BOOL ok = BrotliEncoderCompress(
            quality, window, BROTLI_MODE_GENERIC,
            in_len, in, &out_size,
            reinterpret_cast<uint8_t*>(&buf[0])
        );
        if (!ok) throw std::runtime_error("brotli compress failed");
        buf.resize(out_size);

        return b64_encode(buf);
    }

    // ----- Brotli + hex wrappers -----

    std::string brotli_ascii_compress(const std::string& input, int quality, int window)
    {
        size_t cap = BrotliEncoderMaxCompressedSize(input.size());
        if (cap == 0) cap = input.size() + 512; // fallback for very small inputs

        std::string comp(cap, '\0');
        size_t out_size = cap;

        BROTLI_BOOL ok = BrotliEncoderCompress(
            quality,                               // 0..11
            window,                                // 10..24
            BROTLI_MODE_GENERIC,
            input.size(),
            reinterpret_cast<const uint8_t*>(input.data()),
            &out_size,
            reinterpret_cast<uint8_t*>(&comp[0])
        );
        if (!ok) throw std::runtime_error("brotli compress failed");
        comp.resize(out_size);

        return b64_encode(comp);
    }

    std::string brotli_ascii_decompress(const std::string& ascii)
    {
        std::string comp = b64_decode(ascii);

        // Only grow on demand
        std::string out(std::max<size_t>(comp.size() * 3 + 1024, 1024), '\0');

        BrotliDecoderState* st = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
        if (!st) throw std::runtime_error("brotli state alloc failed");

        const uint8_t* next_in = reinterpret_cast<const uint8_t*>(comp.data());
        size_t avail_in = comp.size();

        uint8_t* next_out = reinterpret_cast<uint8_t*>(&out[0]);
        size_t avail_out = out.size();

        while (true)
        {
            BrotliDecoderResult r = BrotliDecoderDecompressStream(
                st, &avail_in, &next_in, &avail_out, &next_out, nullptr);

            if (r == BROTLI_DECODER_RESULT_SUCCESS) break;
            if (r == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT)
            {
                size_t used = static_cast<size_t>(next_out - reinterpret_cast<uint8_t*>(&out[0]));
                out.resize(out.size() * 2);
                next_out = reinterpret_cast<uint8_t*>(&out[0]) + used;
                avail_out = out.size() - used;
                continue;
            }

            BrotliDecoderDestroyInstance(st);
            throw std::runtime_error("brotli decompress failed");
        }

        size_t produced = static_cast<size_t>(next_out - reinterpret_cast<uint8_t*>(&out[0]));
        out.resize(produced);
        BrotliDecoderDestroyInstance(st);
        return out;
    }

} // namespace Compression
