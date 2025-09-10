#pragma once
#include <bitloop/core/debug.h>
#include <iomanip>
#include <string>
#include <string_view>
#include <sstream>


BL_BEGIN_NS;

namespace TextUtil
{
    template<typename T>
    std::string floatToCleanString(T value, int max_decimal_places, T precision, bool minimize=true)
    {
        // Optional snapping
        if (precision > 0)
            value = round(value / precision) * precision;

        // Use fixed formatting
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(max_decimal_places) << value;
        std::string str = oss.str();

        // Remove trailing zeros
        size_t dot = str.find('.');
        if (dot != std::string::npos)
        {
            size_t last_non_zero = str.find_last_not_of('0');
            if (last_non_zero != std::string::npos)
                str.erase(last_non_zero + 1);

            // Remove trailing dot
            if (str.back() == '.')
                str.pop_back();
        }

        if (minimize)
        {
            if (str == "-0")
                str = "0";

            // Remove the leading '0' for values between -1 and 1
            // (e.g. 0.7351 -> .7351)
            if (!str.empty())
            {
                bool negative = (str[0] == '-');
                std::size_t first = negative ? 1 : 0;
                if (first + 1 < str.size() && str[first] == '0' && str[first + 1] == '.')
                    str.erase(first, 1);
            }
        }

        return str;
    }

    // Wrapping/Unwrapping strings with '\n' at given length
    std::string wrapString(std::string_view input, size_t width);
    std::string unwrapString(std::string_view input);

    std::string indent(std::string_view text, int count = 1, std::string_view indent_unit = "    ", bool indent_empty = false);
    std::string indent_cols(std::string_view text, int cols, bool indent_empty = false);

    std::string dedent(std::string_view text, int tab_width = 4);
    std::string dedent_max(std::string_view text);

    std::string_view trim_view(std::string_view text);

    bool contains_only(const std::string& s, const std::string& allowed);
}

BL_END_NS;
