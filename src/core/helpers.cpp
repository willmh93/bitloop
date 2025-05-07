#include "helpers.h"
#include <iomanip>
#include <sstream>

namespace Helpers {

std::string floatToCleanString(float value, int max_decimal_places, float precision, bool minimize)
{
    // Optional snapping
    if (precision > 0.0f) {
        value = std::round(value / precision) * precision;
    }

    // Use fixed formatting
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(max_decimal_places) << value;
    std::string str = oss.str();

    // Remove trailing zeros
    size_t dot = str.find('.');
    if (dot != std::string::npos) {
        size_t last_non_zero = str.find_last_not_of('0');
        if (last_non_zero != std::string::npos) {
            str.erase(last_non_zero + 1);
        }
        // Remove trailing dot
        if (str.back() == '.') {
            str.pop_back();
        }
    }

    if (minimize) 
    {
        // 4a. Collapse “-0” to “0”
        if (str == "-0") {
            str = "0";
        }

        // 4b. Remove the leading ‘0’ for numbers between -1 and 1
        //     (e.g. "0.7351"  -> ".7351",   "-0.42" -> "-.42")
        if (!str.empty()) {
            bool negative = (str[0] == '-');
            std::size_t first = negative ? 1 : 0;
            if (first + 1 < str.size() && str[first] == '0' && str[first + 1] == '.') {
                str.erase(first, 1);
            }
        }
    }

    return str;
}

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