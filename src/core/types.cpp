#include "types.h"
#include "math_helpers.h"

std::string to_string(const DoubleDouble& x)
{
    if (x.hi == 0.0) return "0";

    // --- 1. get sign and magnitude ----------------------------------------
    DoubleDouble v = x;
    bool neg = false;
    if (v.hi < 0) { neg = true; v.hi = -v.hi; v.lo = -v.lo; }

    // --- 2. scale into [1,10) and find exponent ---------------------------
    int exp10 = static_cast<int>(std::floor(std::log10(v.hi)));
    DoubleDouble scale = std::pow(10.0, -exp10);   // 10^(-exp10)
    v *= scale;                                    // now 1 ≤ v < 10

    // --- 3. generate 32 digits (safely > 31) ------------------------------
    std::ostringstream oss;
    oss << std::setprecision(32) << std::fixed;
    for (int i = 0; i < 32; ++i) {
        int d = static_cast<int>(v.hi);
        oss << d;
        v -= DoubleDouble(d);              // remove integer part
        v *= 10.0;                         // shift left
    }

    std::string s = oss.str();

    // --- 4. insert decimal point after first digit ------------------------
    s.insert(1, ".");
    if (neg) s.insert(0, "-");

    // --- 5. add exponent ---------------------------------------------------
    s += "e";
    s += std::to_string(exp10);

    return s;
}

std::ostream& operator<<(std::ostream& os, const DoubleDouble& x)
{
    return os << to_string(x);
}