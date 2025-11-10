#pragma once

#include <bitloop/core/debug.h>
#include <bitloop/platform/platform_macros.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <functional>
#include <bit>
#include <span>
#include <string_view>

BL_BEGIN_NS;

struct Hashable
{
    virtual ~Hashable() = default;

    std::size_t hash() const noexcept {
        if (!cache_valid) {
            cache = compute_hash();
            cache_valid = true;
        }
        return cache;
    }

    void invalidate_hash() const noexcept { cache_valid = false; }

protected:

    virtual std::size_t compute_hash() const noexcept = 0;

    // ---- utilities ----

    // mixer
    static inline void hash_combine(std::size_t& seed, std::size_t value) noexcept {
        seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
    }

    // hash a single value (floats by bits, enums by underlying, others via std::hash)
    template <class T>
    static inline std::size_t hash_one(const T& v) noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            using I = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
            const I bits = std::bit_cast<I>(v);
            return std::hash<I>{}(bits);
        }
        else if constexpr (std::is_enum_v<T>) {
            using U = std::underlying_type_t<T>;
            return std::hash<U>{}(static_cast<U>(v));
        }
        else {
            return std::hash<T>{}(v);
        }
    }

    // hash a pair by combining items
    template <class A, class B>
    static inline std::size_t hash_pair(const A& a, const B& b) noexcept {
        std::size_t seed = hash_one(a);
        hash_combine(seed, hash_one(b));
        return seed;
    }

    // iterator range
    template <class It>
    static inline std::size_t hash_range(It first, It last) noexcept {
        std::size_t seed = 0;
        for (; first != last; ++first)
            hash_combine(seed, hash_one(*first));
        return seed;
    }

    // span
    template <class T>
    static inline std::size_t hash_span(std::span<const T> s) noexcept {
        return hash_range(s.begin(), s.end());
    }

    // string
    static inline std::size_t hash_string(std::string_view s) noexcept {
        return std::hash<std::string_view>{}(s);
    }

    // raw bytes
    static inline std::size_t hash_bytes(const void* data, std::size_t n) noexcept {
        const auto* p = static_cast<const unsigned char*>(data);
        std::size_t seed = 0;
        for (std::size_t i = 0; i < n; ++i)
            hash_combine(seed, std::hash<unsigned char>{}(p[i]));
        return seed;
    }

private:
    mutable std::size_t cache{ 0 };
    mutable bool cache_valid{ false };
};

BL_END_NS;
