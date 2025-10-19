#include <bitloop/util/compression.h>

#include <brotli/encode.h>
#include <brotli/decode.h>
#include <stdexcept>
#include <vector>
#include <cctype>

namespace
{
    constexpr char lookup[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";


    inline unsigned char b62_value(unsigned char c)
    {
        if (c >= 'A' && c <= 'Z') return static_cast<unsigned char>(c - 'A');
        if (c >= 'a' && c <= 'z') return static_cast<unsigned char>(26 + (c - 'a'));
        if (c >= '0' && c <= '9') return static_cast<unsigned char>(52 + (c - '0'));
        return 0xFF; // invalid
    }

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

    std::string b62_encode(std::string_view bytes)
    {
        static constexpr char lut[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"; // 62 symbols

        if (bytes.empty()) return {};

        std::vector<unsigned char> n(bytes.begin(), bytes.end());
        size_t zeros = 0;
        while (zeros < n.size() && n[zeros] == 0) ++zeros;

        std::vector<unsigned char> enc;
        enc.reserve(n.size() * 2 / 3 + 1);

        size_t start = zeros, end = n.size();
        while (start < end) {
            unsigned int carry = 0;
            for (size_t i = start; i < end; ++i) {
                unsigned int val = unsigned(n[i]) + (carry << 8); // *256
                n[i] = static_cast<unsigned char>(val / 62);
                carry = val % 62;
            }
            enc.push_back(static_cast<unsigned char>(carry));
            while (start < end && n[start] == 0) ++start;
        }

        std::string out;
        out.reserve(zeros + enc.size());
        for (size_t i = 0; i < zeros; ++i) out.push_back(lut[0]);
        for (auto it = enc.rbegin(); it != enc.rend(); ++it) out.push_back(lut[*it]);
        return out;
    }

    // ========== Base64 decode ==========
    bool valid_b62(std::string_view s)
    {
        for (char c : s) {
            if (b62_value(c) == 255) return false;
        }
        return true;

    }

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

    std::string b62_decode(std::string_view s)
    {
        // Must match the encoder's alphabet and ordering
        static int8_t map[256];
        static bool inited = false;
        if (!inited) {
            for (int i = 0; i < 256; ++i) map[i] = -1;
            for (int i = 0; i < 26; ++i) map[static_cast<unsigned>('A') + i] = i;        // 0..25
            for (int i = 0; i < 26; ++i) map[static_cast<unsigned>('a') + i] = 26 + i;  // 26..51
            for (int i = 0; i < 10; ++i) map[static_cast<unsigned>('0') + i] = 52 + i;  // 52..61
            inited = true;
        }

        if (s.empty()) return {};

        // Count leading zero digits (alphabet[0] == 'A') -> leading zero bytes
        size_t zeros = 0;
        while (zeros < s.size()) {
            unsigned char c = static_cast<unsigned char>(s[zeros]);
            int8_t v = map[c];
            if (v < 0) throw std::runtime_error("invalid base62 char");
            if (v != 0) break;
            ++zeros;
        }

        // Convert Base62 digits (after leading zeros) to Base256 via repeated division by 256
        std::vector<unsigned char> digits;
        digits.reserve(s.size() - zeros);
        for (size_t i = zeros; i < s.size(); ++i) {
            int8_t v = map[static_cast<unsigned char>(s[i])];
            if (v < 0) throw std::runtime_error("invalid base62 char");
            digits.push_back(static_cast<unsigned char>(v));
        }

        std::vector<unsigned char> dec;
        dec.reserve(digits.size());
        size_t start = 0, end = digits.size();
        while (start < end) {
            unsigned int carry = 0;
            for (size_t i = start; i < end; ++i) {
                unsigned int val = static_cast<unsigned int>(digits[i]) + carry * 62u;
                digits[i] = static_cast<unsigned char>(val / 256u);
                carry = val % 256u;
            }
            dec.push_back(static_cast<unsigned char>(carry));
            while (start < end && digits[start] == 0) ++start;
        }

        // Assemble output: leading zero bytes + reversed collected bytes
        std::string out;
        out.reserve(zeros + dec.size());
        out.append(zeros, '\0');
        for (auto it = dec.rbegin(); it != dec.rend(); ++it)
            out.push_back(static_cast<char>(*it));
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

        return b62_encode(comp);
    }

    std::string brotli_ascii_decompress(const std::string& ascii)
    {
        std::string comp = b62_decode(ascii);

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
