#pragma once
#include <vector>
#include <string>
#include <stdexcept>

using ByteVec = std::vector<uint8_t>;

namespace Encoding
{
    // ===================== Base 64 encoding =======================
    static const char* base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

    inline unsigned int indexOf(char c)
    {
        const char* pos = std::strchr(base64_chars, c);
        if (!pos) throw std::runtime_error("Invalid character in Base64 string");
        return static_cast<unsigned int>(pos - base64_chars);
    }

    std::string base64_encode(const uint8_t* data, size_t len);
    ByteVec     base64_decode(const std::string& encoded);

    // ===================== Compression =======================
    class Bitstream
    {
        std::vector<uint8_t> data;

    public:

        uint8_t* getData() { return data.data(); }
        size_t getSize() { return data.size(); }
        size_t getBitCount() { return data.size() * 8; }

        bool getbit(uint8_t x, int y) {
            return (x >> (7 - y)) & 1;
        }
        int chbit(int x, int i, bool v) {
            if (v) return x | (1 << (7 - i));
            return x & ~(1 << (7 - i));
        }

        // opens an existing byte array as bitstream
        void openBytes(uint8_t* bytes, size_t _size);

        // read/write to bitstream
        void write(size_t ind, size_t bits, int dat, bool print = false);
        int read(size_t ind, size_t bits, bool print = false);

        //void print_bits(size_t bitIndex, size_t bitCount);
        //void print(size_t grouping = 8);
    };

    void compressBitstream(const std::string& _uncompressed, Bitstream& result);
    void compressBitstream(const ByteVec& input, Bitstream& out);

    std::string decompressBitstream(Bitstream& data);
    ByteVec     decompressBitstreamBytes(Bitstream& in);

    // LZW compression - Works with strings/binary data
    std::string base64_compress(const std::string& data);
    std::string base64_decompress(const std::string& base64_txt);
}