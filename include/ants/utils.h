// Miscellaneous structs
// Matt Young, 2022
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <ostream>

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
    };

    /// 2D vector (integer)
    // FIXME make this a template?
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
    };

    /// 2D vector (float)
    struct Vector2f {
        float x{}, y{};

        Vector2f() = default;

        Vector2f(float x, float y) : x(x), y(y) {}

        bool operator==(const Vector2f &rhs) const {
            return x == rhs.x &&
                   y == rhs.y;
        }

        bool operator!=(const Vector2f &rhs) const {
            return !(rhs == *this);
        }

        bool operator<(const Vector2f &rhs) const {
            if (x < rhs.x)
                return true;
            if (rhs.x < x)
                return false;
            return y < rhs.y;
        }

        bool operator>(const Vector2f &rhs) const {
            return rhs < *this;
        }

        bool operator<=(const Vector2f &rhs) const {
            return !(rhs < *this);
        }

        bool operator>=(const Vector2f &rhs) const {
            return !(*this < rhs);
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
    struct std::hash<ants::RGBColour>
    {
        std::size_t operator()(const ants::RGBColour& c) const
        {
            size_t seed = c.r;
            ants::hashCombine(seed, c.g);
            ants::hashCombine(seed, c.b);
            return seed;
        }
    };
};