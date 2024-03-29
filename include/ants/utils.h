// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <ostream>
#include <cmath>
#include "cereal/cereal.hpp"

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

        // for cereal
        friend class cereal::access;
        template<class Archive>
        void serialize(Archive & archive) {
            archive(CEREAL_NVP(r), CEREAL_NVP(g), CEREAL_NVP(b));
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

        // for cereal
        friend class cereal::access;
        template<class Archive>
        void serialize(Archive & archive) {
            archive(CEREAL_NVP(x), CEREAL_NVP(y));
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

    /**
     * Computes the CRC32 hash of the buffer.
     * @param buf contiguous buffer
     * @param size size of buf in bytes
     * @return CRC32 sum of the buffer
     */
    uint32_t crc32(const void *buf, size_t size);

    /**
     * Prints the hex dump of the given buffer
     * Source: https://stackoverflow.com/a/29865/5007892
     */
    void hexdump(const void *ptr, size_t size);
}

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