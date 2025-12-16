#pragma once

#include <bitloop/core/debug.h>
#include <bitloop/platform/platform_macros.h>

#include <bit>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>

BL_BEGIN_NS;

using hash_t = std::uint64_t;

struct Hashable
{
    virtual ~Hashable() = default;

    // Stable, cross-platform hash value (persist THIS one)
    hash_t stable_hash() const noexcept {
        if (!cache_valid) {
            cache = compute_hash();
            cache_valid = true;
        }
        return cache;
    }

    // Convenience for unordered_* on this platform (do NOT persist)
    std::size_t hash() const noexcept {
        hash_t h = stable_hash();
        if constexpr (sizeof(std::size_t) >= sizeof(hash_t))
            return static_cast<std::size_t>(h);
        else
            return static_cast<std::size_t>(h ^ (h >> 32)); // fold to 32-bit
    }

    void invalidate_hash() const noexcept { cache_valid = false; }

protected:
    virtual hash_t compute_hash() const noexcept = 0;

    // ---- utilities ----

    static inline hash_t mix64(hash_t x) noexcept {
        // SplitMix64 finalizer (well-known avalanche)
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ull;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebull;
        x ^= x >> 31;
        return x;
    }

    static inline void hash_combine(hash_t& seed, hash_t v) noexcept {
        // Order-sensitive combine
        seed = mix64(seed ^ mix64(v + 0x9e3779b97f4a7c15ull));
    }

    static inline hash_t hash_bytes(const void* data, std::size_t n) noexcept {
        // FNV-1a 64-bit over raw bytes, then avalanche
        const auto* p = static_cast<const std::uint8_t*>(data);
        hash_t h = 14695981039346656037ull; // offset basis
        for (std::size_t i = 0; i < n; ++i) {
            h ^= static_cast<hash_t>(p[i]);
            h *= 1099511628211ull; // FNV prime
        }
        return mix64(h ^ static_cast<hash_t>(n));
    }

    static inline hash_t hash_string(std::string_view s) noexcept {
        return hash_bytes(s.data(), s.size());
    }

    template <class T>
    static inline hash_t hash_one(const T& v) noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            using I = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
            I bits = std::bit_cast<I>(v);

            // Canonicalize -0.0 -> +0.0
            if constexpr (sizeof(T) == 4) {
                if ((bits & 0x7fffffffu) == 0) bits = 0;
                // Canonicalize all NaNs
                if ((bits & 0x7f800000u) == 0x7f800000u && (bits & 0x007fffffu))
                    bits = 0x7fc00000u;
            }
            else {
                if ((bits & 0x7fffffffffffffffull) == 0) bits = 0;
                if ((bits & 0x7ff0000000000000ull) == 0x7ff0000000000000ull && (bits & 0x000fffffffffffffull))
                    bits = 0x7ff8000000000000ull;
            }

            return mix64(static_cast<hash_t>(bits));
        }
        else if constexpr (std::is_enum_v<T>) {
            using U = std::underlying_type_t<T>;
            return hash_one(static_cast<U>(v));
        }
        else if constexpr (std::is_integral_v<T> || std::is_pointer_v<T>) {
            // Pointers are process-specific; only hash pointers if you never persist/compare across runs.
            return mix64(static_cast<hash_t>(v));
        }
        else {
            // For non-trivial types, hash their bytes ONLY if they have a stable serialized form.
            // Otherwise, define a specific hashing routine for that type.
            static_assert(std::is_trivially_copyable_v<T>,
                "hash_one(T): Provide a stable hashing implementation for this type.");
            return hash_bytes(&v, sizeof(T));
        }
    }

    template <class A, class B>
    static inline hash_t hash_pair(const A& a, const B& b) noexcept {
        hash_t seed = 0;
        hash_combine(seed, hash_one(a));
        hash_combine(seed, hash_one(b));
        return seed;
    }

    template <class It>
    static inline hash_t hash_range(It first, It last) noexcept {
        hash_t seed = 0;
        hash_t count = 0;
        for (; first != last; ++first, ++count)
            hash_combine(seed, hash_one(*first));
        hash_combine(seed, count); // length matters
        return seed;
    }

    template <class T>
    static inline hash_t hash_span(std::span<const T> s) noexcept {
        return hash_range(s.begin(), s.end());
    }

private:
    mutable hash_t cache{ 0 };
    mutable bool cache_valid{ false };
};
BL_END_NS;
