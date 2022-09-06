// Miscellaneous structs
// Matt Young, 2022
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <ostream>
#include <cmath>

/// Divide to convert bytes to MiB
#define BYTES2MIB 1048576

namespace ants {
    // Source: https://stackoverflow.com/a/2595226/5007892
    template <class T>
    inline void hashCombine(std::size_t& seed, const T& v) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    /// RGB colour
    struct RGBColour {
        uint8_t r{}, g{}, b{};

        RGBColour() = default;

        RGBColour(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

        bool operator==(const RGBColour &rhs) const {
            return r == rhs.r &&
                   g == rhs.g &&
                   b == rhs.b;
        }

        bool operator!=(const RGBColour &rhs) const {
            return !(rhs == *this);
        }

        bool operator<(const RGBColour &rhs) const {
            if (r < rhs.r)
                return true;
            if (rhs.r < r)
                return false;
            if (g < rhs.g)
                return true;
            if (rhs.g < g)
                return false;
            return b < rhs.b;
        }

        bool operator>(const RGBColour &rhs) const {
            return rhs < *this;
        }

        bool operator<=(const RGBColour &rhs) const {
            return !(rhs < *this);
        }

        bool operator>=(const RGBColour &rhs) const {
            return !(*this < rhs);
        }

        friend std::ostream &operator<<(std::ostream &os, const RGBColour &colour) {
            os << "(" << std::to_string(colour.r) << ", " << std::to_string(colour.g)
                << ", " << std::to_string(colour.b) << ")";
            return os;
        }

        RGBColour operator*(double x) const {
            auto newR = static_cast<uint8_t>(std::round(static_cast<double>(r) * x));
            auto newG = static_cast<uint8_t>(std::round(static_cast<double>(g) * x));
            auto newB = static_cast<uint8_t>(std::round(static_cast<double>(b) * x));
            return {newR, newG, newB};
        }
    };

    /// 2D vector (integer)
    struct Vector2i {
        int32_t x{}, y{};

        Vector2i() = default;

        Vector2i(int32_t x, int32_t y) : x(x), y(y) {}

        bool operator==(const Vector2i &rhs) const {
            return x == rhs.x &&
                   y == rhs.y;
        }

        bool operator!=(const Vector2i &rhs) const {
            return !(rhs == *this);
        }

        bool operator<(const Vector2i &rhs) const {
            if (x < rhs.x)
                return true;
            if (rhs.x < x)
                return false;
            return y < rhs.y;
        }

        bool operator>(const Vector2i &rhs) const {
            return rhs < *this;
        }

        bool operator<=(const Vector2i &rhs) const {
            return !(rhs < *this);
        }

        bool operator>=(const Vector2i &rhs) const {
            return !(*this < rhs);
        }

        /// Chebyshev distance between this and another vector
        [[nodiscard]] int32_t distance(Vector2i other) const {
            // https://en.wikipedia.org/wiki/Chebyshev_distance#Definition
            return std::max(std::abs(other.x - x), std::abs(other.y - y));
        }

        Vector2i operator-(const Vector2i &other) const {
            return {x - other.x, y - other.y};
        }

        /// Normalises this vector and returns a new vector
        [[nodiscard]] Vector2i norm() const {
            // instead of Euclidean distance, take the Chebyshev distance as the norm since we are
            // on a grid
            // this is absolutely critical to making it work!!! for some reason
            auto len = static_cast<double>(std::max(std::abs(x), std::abs(y)));
            if (len != 0) {
                auto x2 = x / static_cast<int>(round(len));
                auto y2 = y / static_cast<int>(round(len));
                return {x2, y2};
            }
            return {0, 0}; // hack, stupid, should fix
        }
    };

    /// Stores a millisecond time and frames per second measure
    struct TimeInfo {
        double timeMs{};
        double fps{};

        TimeInfo(double timeMs, double fps) : timeMs(timeMs), fps(fps) {}

        friend std::ostream &operator<<(std::ostream &os, const TimeInfo &info) {
            os << info.timeMs << "ms (" << info.fps << " ticks per second)";
            return os;
        }
    };
};

namespace std {
    /// Hash for RGBColour
    template <>
    struct hash<ants::RGBColour>
    {
        std::size_t operator()(const ants::RGBColour& c) const
        {
            size_t hash = 0;
            ants::hashCombine(hash, c.r);
            ants::hashCombine(hash, c.g);
            ants::hashCombine(hash, c.b);
            return hash;
        }
    };

    /// Hash for Vector2i
    template <>
    struct hash<ants::Vector2i>
    {
        std::size_t operator()(const ants::Vector2i& v) const
        {
            size_t hash = 0;
            ants::hashCombine(hash, v.x);
            ants::hashCombine(hash, v.y);
            return hash;
        }
    };
};