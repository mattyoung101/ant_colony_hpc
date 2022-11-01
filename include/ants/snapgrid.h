// Snapshot grid (SnapGrid) as documented in docs/parallel.md
// Matt Young, 2022
#pragma once
#include <cstdint>
#include <cstring>
#include "log/log.h"

// Indexing: https://softwareengineering.stackexchange.com/a/212813

namespace ants {
    /// 2D snapshot grid, as documented in docs/parallel.md
    template<typename T>
    struct SnapGrid2D {
        /// Constructs a new empty SnapGrid
        explicit SnapGrid2D(int32_t width, int32_t height)  {
            this->width = width;
            this->height = height;
            log_debug("new SnapGrid2D, width: %d, height: %d, array size: %d, bytes: %lu", width,
                      height, width * height, width * height * sizeof(T));
            log_debug("SnapGrid2D sizeof(T): %lu", sizeof(T));
            clean = new T[width * height]{};
            dirty = new T[width * height]{};
        }

        SnapGrid2D() = default;

        /// Writes a value into the dirty buffer
        inline constexpr void write(int32_t x, int32_t y, T value) {
            dirty[x + width * y] = value;
        }


        /// Reads a value from the snapshot grid, from the clean buffer
        inline constexpr T read(int32_t x, int32_t y) const {
            return clean[x + width * y];
        }

        /**
         * Commits the dirty buffer, i.e. replaces the current clean buffer with the current dirty
         * buffer.
         */
        inline constexpr void commit() {
            memcpy(clean, dirty, width * height * sizeof(T));
        }

        /// Computes the CRC32 hash of the dirty buffer. Used for data verification.
        inline constexpr uint32_t crc32Dirty() {
            return crc32(dirty, width * height * sizeof(T));
        }

        /// Computes the CRC32 hash of the clean buffer. Used for data verification.
        inline constexpr uint32_t crc32Clean() {
            return crc32(clean, width * height * sizeof(T));
        }

        /// Clean buffer
        T *clean{};
        /// Dirty buffer
        T *dirty{};
        int32_t width{}, height{};
    };


    /// 3D snapshot grid, as documented in docs/parallel.md
    template<typename T>
    struct SnapGrid3D {
        /// Constructs a new empty SnapGrid
        explicit SnapGrid3D(int32_t width, int32_t height, int32_t depth)  {
            this->width = width;
            this->height = height;
            this->depth = depth;
            log_debug("new SnapGrid3D, width: %d, height: %d, depth: %d, array size: %d, bytes: %lu", width,
                      height, depth, width * height * depth, width * height * depth * sizeof(T));
            log_debug("SnapGrid3D sizeof(T): %lu", sizeof(T));
            clean = new T[width * height * depth];
            dirty = new T[width * height * depth];
        }

        SnapGrid3D() = default;

        /// Writes a value into the dirty buffer
        template<class I>
        inline constexpr void write(int32_t x, int32_t y, I z, T value) {
            dirty[x + width * y + width * height * z] = value;
        }

        /// Reads a value from the snapshot grid, from the clean buffer
        template<class I>
        inline constexpr T read(int32_t x, int32_t y, I z) const {
            return clean[x + width * y + width * height * z];
        }

        /**
         * Commits the dirty buffer, i.e. replaces the current clean buffer with the current dirty
         * buffer.
         */
        inline constexpr void commit() {
            memcpy(clean, dirty, width * height * depth * sizeof(T));
        }

        /// Clean buffer
        T *clean{};
        /// Dirty buffer
        T *dirty{};
        int32_t width{}, height{}, depth{};
    };
}