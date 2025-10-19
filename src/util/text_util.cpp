#include <bitloop/util/text_util.h>
#include <unordered_set>

std::ostream& operator<<(std::ostream& os, const FiniteDouble& fd) {
    return os << fd.value;
}

BL_BEGIN_NS;

namespace TextUtil {

std::string wrapString(std::string_view input, size_t width)
{
    std::string output;
    for (size_t i = 0; i < input.size(); i++) {
        if (i > 0 && i % width == 0)
            output += '\n';
        output += input[i];
    }
    return output;
}

std::string unwrapString(std::string_view input)
{
    std::string output;
    output.reserve(input.size());

    for (char c : input) {
        if (c != '\n')
            output += c;
    }

    return output;
}

// Return true if line (without its EOL) is all spaces/tabs or empty.
bool is_blank_line(std::string_view line)
{
    for (char c : line) {
        if (c != ' ' && c != '\t') return false;
    }
    return true;
}

// Count displayed "columns" of leading whitespace of a line, expanding tabs.
std::size_t leading_columns(std::string_view line, int tab_width)
{
    std::size_t cols = 0;
    for (char c : line)
    {
        if (c == ' ')
            ++cols;
        else if (c == '\t')
        {
            int tw = tab_width > 0 ? tab_width : 4;
            std::size_t advance = tw - (cols % static_cast<std::size_t>(tw));
            cols += advance;
        }
        else break;
    }
    return cols;
}

// How many characters from the start of `line` remove to consume `cols_to_remove` columns.
std::size_t chars_to_consume_for_columns(std::string_view line, std::size_t cols_to_remove, int tab_width)
{
    std::size_t cols = 0;
    std::size_t i = 0;
    int tw = tab_width > 0 ? tab_width : 4;
    while (i < line.size() && cols < cols_to_remove)
    {
        char c = line[i];
        if (c == ' ')
        {
            ++cols;
            ++i;
        }
        else if (c == '\t')
        {
            std::size_t advance = tw - (cols % static_cast<std::size_t>(tw));
            cols += advance;
            ++i;
        }
        else
        {
            break;
        }
    }
    return i;
}

// Compute minimal common indentation (in columns) across non-blank lines.
std::size_t min_indent_columns(std::string_view text, int tab_width)
{
    std::size_t min_cols = std::numeric_limits<std::size_t>::max();

    std::size_t i = 0;
    while (i <= text.size())
    {
        // find end of line
        std::size_t line_start = i;
        std::size_t nl = text.find('\n', i);
        bool has_nl = nl != std::string_view::npos;
        std::size_t line_end = has_nl ? nl : text.size();

        // handle optional preceding '\r'
        std::size_t content_end = line_end;
        if (content_end > line_start && text[content_end - 1] == '\r') {
            --content_end;
        }

        std::string_view line = text.substr(line_start, content_end - line_start);
        if (!is_blank_line(line)) {
            std::size_t cols = leading_columns(line, tab_width);
            if (cols < min_cols) min_cols = cols;
        }

        if (!has_nl) break;
        i = nl + 1;
    }

    if (min_cols == std::numeric_limits<std::size_t>::max()) return 0; // all blank
    return min_cols;
}

// Indent every line by repeating `indent_unit` `count` times.
// If indent_empty == false, blank lines are not indented.
std::string indent(std::string_view text, int count, std::string_view indent_unit, bool indent_empty)
{
    if (count <= 0 || indent_unit.empty())
        return std::string{ text };

    std::string prefix;
    prefix.reserve(indent_unit.size() * static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) prefix += indent_unit;

    std::string out;
    out.reserve(text.size() + prefix.size() * 8); // rough guess

    std::size_t i = 0;
    while (i <= text.size())
    {
        std::size_t line_start = i;
        std::size_t nl = text.find('\n', i);
        bool has_nl = nl != std::string_view::npos;
        std::size_t line_end = has_nl ? nl : text.size();

        bool had_cr = (line_end > line_start && text[line_end - 1] == '\r');
        std::size_t content_end = had_cr ? (line_end - 1) : line_end;

        std::string_view line = text.substr(line_start, content_end - line_start);

        if (indent_empty || !is_blank_line(line)) out += prefix;
        out.append(line.data(), line.size());

        if (!has_nl) break;
        if (had_cr) out += "\r\n"; else out += '\n';
        i = nl + 1;
    }
    return out;
}

// Indent by N columns using spaces. For tabs, pass indent() with indent_unit = "\t".
std::string indent_cols(std::string_view text, int cols, bool indent_empty)
{
    if (cols <= 0) return std::string{ text };
    return indent(text, 1, std::string(static_cast<std::size_t>(cols), ' '), indent_empty);
}

// Dedent by the minimal common leading indentation (in columns), skipping blank lines.
// Tabs are expanded using tab_width when measuring/removing indentation.
std::string dedent(std::string_view text, int tab_width)
{
    std::size_t remove_cols = min_indent_columns(text, tab_width);
    if (remove_cols == 0) return std::string{ text };

    std::string out;
    out.reserve(text.size());

    std::size_t i = 0;
    while (i <= text.size())
    {
        std::size_t line_start = i;
        std::size_t nl = text.find('\n', i);
        bool has_nl = nl != std::string_view::npos;
        std::size_t line_end = has_nl ? nl : text.size();

        bool had_cr = (line_end > line_start && text[line_end - 1] == '\r');
        std::size_t content_end = had_cr ? (line_end - 1) : line_end;

        std::string_view line = text.substr(line_start, content_end - line_start);

        if (is_blank_line(line))
        {
            // Leave blank lines untouched
            out.append(line.data(), line.size());
        }
        else
        {
            std::size_t consume = chars_to_consume_for_columns(line, remove_cols, tab_width);
            out.append(line.data() + consume, line.size() - consume);
        }

        if (!has_nl) break;
        if (had_cr) out += "\r\n"; else out += '\n';
        i = nl + 1;
    }
    return out;
}

std::string dedent_max(std::string_view text)
{
    std::string out;
    out.reserve(text.size());

    std::size_t i = 0;
    while (i <= text.size())
    {
        std::size_t line_start = i;
        std::size_t nl = text.find('\n', i);
        bool has_nl = nl != std::string_view::npos;
        std::size_t line_end = has_nl ? nl : text.size();

        bool had_cr = (line_end > line_start && text[line_end - 1] == '\r');
        std::size_t content_end = had_cr ? (line_end - 1) : line_end;

        std::string_view line = text.substr(line_start, content_end - line_start);

        // Skip all leading spaces/tabs
        std::size_t consume = 0;
        while (consume < line.size() && (line[consume] == ' ' || line[consume] == '\t'))
            ++consume;

        out.append(line.data() + consume, line.size() - consume);

        if (!has_nl) break;
        if (had_cr) out += "\r\n"; else out += '\n';
        i = nl + 1;
    }
    return out;
}

std::string_view trim_view(std::string_view text)
{
    // left
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
        text.remove_prefix(1);
    // right
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
        text.remove_suffix(1);
    return text;
}

bool contains_only(const std::string& s, const std::string& allowed)
{
    std::unordered_set<char> allowedSet(allowed.begin(), allowed.end());
    for (char c : s) {
        if (allowedSet.find(c) == allowedSet.end()) {
            return false; // found a char not in allowed
        }
    }
    return true;
}

} // End TextUtil NS

BL_END_NS;
