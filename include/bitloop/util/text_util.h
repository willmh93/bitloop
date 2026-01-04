#pragma once
#include <bitloop/core/debug.h>
#include <iomanip>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>


BL_BEGIN_NS;

typedef std::vector<std::string_view> string_view_list;

namespace text
{

    // Helper to convert floating-point number to a clean formatted string
    // - with optional snapping / trim leading/trailing 0's
    template<typename T>
    std::string floatToCleanString(T value, int max_decimal_places, T snap_size, bool trim_trailing_zeros=true, bool trim_leading_zero=false)
    {
        // Optional snapping
        if (snap_size > 0)
            value = round(value / snap_size) * snap_size;

        // Use fixed formatting
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(max_decimal_places) << value;
        std::string str = oss.str();

        // Remove trailing zeros
        if (trim_trailing_zeros)
        {
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
        }

        if (str == "-0")
            str = "0";

        if (trim_leading_zero)
        {
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
    std::string indentCols(std::string_view text, int cols, bool indent_empty = false);

    std::string dedent(std::string_view text, int tab_width = 4);
    std::string dedentMax(std::string_view text);

    std::string_view trimStringView(std::string_view text);

    bool containsOnly(const std::string& s, const std::string& allowed);

    std::string formatHumanU64(uint64_t value, int sig_figs = 5);

    std::vector<std::string_view> split(std::string_view s, char delim, bool skip_empty = false);

    inline char tolower(char c) { return (char)std::tolower((unsigned char)c); }
    inline bool eqInsensitive(char a, char b) { return tolower(a) == tolower(b); }
    inline bool eqInsensitive(std::string_view a, std::string_view b)
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (!eqInsensitive(a[i], b[i])) return false;
        return true;
    }
}

BL_END_NS;
