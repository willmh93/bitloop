#include "helpers.h"
#include <iomanip>
#include <sstream>

namespace Helpers {



std::string wrapString(const std::string& input, size_t width)
{
    std::string output;
    for (size_t i = 0; i < input.size(); i++) {
        if (i > 0 && i % width == 0)
            output += '\n';
        output += input[i];
    }
    return output;
}

std::string unwrapString(const std::string& input)
{
    std::string output;
    output.reserve(input.size()); // optional: avoid reallocations

    for (char c : input) {
        if (c != '\n')
            output += c;
    }

    return output;
}

} // End Helpers NS