#include "compression.h"
#include <unordered_map>

namespace Compression {

std::string base64_encode(const uint8_t* data, size_t len)
{
    std::string encoded;
    encoded.reserve(((len + 2) / 3) * 4);

    size_t i = 0;
    while (i < len)
    {
        // how many bytes are left in this block?
        size_t remain = len - i;

        unsigned char a = data[i++];
        unsigned char b = (remain > 1) ? data[i++] : 0;
        unsigned char c = (remain > 2) ? data[i++] : 0;

        uint32_t triple = (a << 16) | (b << 8) | c;

        encoded.push_back(base64_chars[(triple >> 18) & 0x3F]);
        encoded.push_back(base64_chars[(triple >> 12) & 0x3F]);

        encoded.push_back((remain > 1)
            ? base64_chars[(triple >> 6) & 0x3F]
            : '=');

        encoded.push_back((remain > 2)
            ? base64_chars[triple & 0x3F]
            : '=');
    }
    return encoded;
}

ByteVec base64_decode(const std::string& encoded)
{
    size_t len = encoded.size();
    if (len % 4 != 0)
        throw std::runtime_error("Invalid Base64 string length");

    // Determine the number of padding characters.
    size_t padding = 0;
    if (len) {
        if (encoded[len - 1] == '=') padding++;
        if (encoded[len - 2] == '=') padding++;
    }

    std::vector<unsigned char> decoded;
    decoded.reserve((len / 4) * 3 - padding);

    // Process every 4 characters as a block.
    for (size_t i = 0; i < len; i += 4) {
        unsigned int sextet_a = (encoded[i] == '=') ? 0 : indexOf(encoded[i]);
        unsigned int sextet_b = (encoded[i + 1] == '=') ? 0 : indexOf(encoded[i + 1]);
        unsigned int sextet_c = (encoded[i + 2] == '=') ? 0 : indexOf(encoded[i + 2]);
        unsigned int sextet_d = (encoded[i + 3] == '=') ? 0 : indexOf(encoded[i + 3]);

        // Pack the 4 sextets into a 24-bit integer.
        unsigned int triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        // Extract the original bytes from the 24-bit number.
        decoded.push_back((triple >> 16) & 0xFF);
        if (encoded[i + 2] != '=')
            decoded.push_back((triple >> 8) & 0xFF);
        if (encoded[i + 3] != '=')
            decoded.push_back(triple & 0xFF);
    }

    return decoded;
}


inline void Bitstream::openBytes(uint8_t* bytes, size_t _size)
{
    data.resize(_size);
    memcpy(data.data(), bytes, _size);
}

inline void Bitstream::write(size_t ind, size_t bits, int dat, bool print)
{
    size_t orig_ind = ind;
    ind += bits - 1;
    size_t max_len = (ind / 8) + 1;
    if (max_len > this->data.size())
        this->data.resize(max_len);

    // update the byte for each right-most bit consumed from dat
    while (dat) {
        data[ind / 8] = chbit(data[ind / 8], ind % 8, dat & 1);
        dat /= 2;
        ind--;
    }
}

inline int Bitstream::read(size_t ind, size_t bits, bool print)
{
    int dat = 0;
    size_t end_ind = ind + bits;
    size_t num_bits = this->getBitCount();
    if (end_ind > num_bits)
        end_ind = num_bits;

    for (size_t i = ind; i < end_ind; i++) {
        dat = dat * 2 + getbit(data[i / 8], i % 8);
    }
    return dat;
}

/*inline void Bitstream::print_bits(size_t bitIndex, size_t bitCount)
{
    size_t first_bit = bitIndex;
    size_t last_bit = bitIndex + bitCount;

    for (size_t i = first_bit; i < last_bit; i++)
        std::cout << read(i, 1) ? '1' : '0';

    std::cout << ' ';
}

inline void Bitstream::print(size_t grouping)
{
    size_t first_bit = 0;
    size_t last_bit = this->getBitCount();

    for (size_t i = first_bit; i < last_bit; i++)
    {
        std::cout << read(i, 1) ? '1' : '0';
        if ((i + 1) % grouping == 0)
            std::cout << ' ';
    }

    std::cout << ' ';
}*/

void compressBitstream(const std::string& _uncompressed, Bitstream& result)
{
    std::string uncompressed = _uncompressed;
    std::unordered_map<std::string, int> dictionary;
    std::string w, wc;
    for (int i = 0; i < 256; i++)
        dictionary[std::string(1, i)] = i;

    size_t len = uncompressed.length();
    const char* str = uncompressed.c_str();
    int dictSize = 256;
    size_t dictBitLen = 9;
    size_t bi = 0;
    size_t i;
    char c;

    for (i = 0; i < len; i++)
    {
        c = str[i];
        wc = w + c;
        if (dictionary.count(wc))
            w = wc;
        else
        {
            result.write(bi, dictBitLen, dictionary[w], true);
            bi += dictBitLen;

            // Add wc to the dictionary.
            dictionary[wc] = dictSize++;
            dictBitLen = (size_t)ceil(log2(dictSize));
            w = std::string(1, c);
        }
    }

    // Output the code for w.
    if (!w.empty())
        result.write(bi, dictBitLen, dictionary[w], true);
}

void compressBitstream(const ByteVec& input, Bitstream& out)
{
    // reinterpret the bytes as a string with the SAME length
    const std::string s(reinterpret_cast<const char*>(input.data()),
        input.size());
    compressBitstream(s, out);                       // reuse your original function
}

std::string decompressBitstream(Bitstream& data)
{
    /* --- build initial dictionary --------------------------------------- */
    int    dictSize = 256;
    size_t dictBitLen = 9;
    std::unordered_map<int, std::string> dictionary;
    for (int i = 0; i < 256; ++i)
        dictionary[i] = std::string(1, static_cast<char>(i));

    /* --- read first code ------------------------------------------------- */
    size_t bi = 0;
    const size_t end = data.getBitCount();
    if (end < dictBitLen)
        return {};                          // empty stream

    int firstCode = data.read(bi, dictBitLen);
    bi += dictBitLen;

    std::string w = dictionary[firstCode];
    std::string result = w;

    /* --- main loop ------------------------------------------------------- */
    while (bi + dictBitLen <= end)          // <= enough bits for one more code
    {
        int k = data.read(bi, dictBitLen);
        bi += dictBitLen;

        std::string entry;
        if (dictionary.count(k))
            entry = dictionary[k];
        else if (k == dictSize)
            entry = w + w[0];
        else
            throw std::runtime_error("Bad LZW code");

        result += entry;
        dictionary[dictSize++] = w + entry[0];
        dictBitLen = static_cast<size_t>(std::ceil(std::log2(dictSize)));

        w = std::move(entry);
    }
    return result;
}

ByteVec decompressBitstreamBytes(Bitstream& in)
{
    std::string s = decompressBitstream(in);
    return ByteVec(s.begin(), s.end());
}

std::string base64_compress(const std::string& txt)
{
    Compression::Bitstream compressed;
    Compression::compressBitstream(txt, compressed);
    std::string base64_txt = Compression::base64_encode(compressed.getData(), compressed.getSize());
    return base64_txt;
}

std::string base64_decompress(const std::string& base64_txt)
{
    ByteVec data = Compression::base64_decode(base64_txt);
    Compression::Bitstream compressed;
    compressed.openBytes(data.data(), data.size());
    return Compression::decompressBitstream(compressed);
}


} // End NS Compression