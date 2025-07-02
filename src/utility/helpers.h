#pragma once
#include "debug.h"




namespace Helpers
{
    template<typename T>
    std::string floatToCleanString(T value, int max_decimal_places, T precision, bool minimize=true)
    {
        // Optional snapping
        if (precision > 0)
            value = std::round(value / precision) * precision;

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
    std::string wrapString(const std::string& input, size_t width);
    std::string unwrapString(const std::string& input);
}