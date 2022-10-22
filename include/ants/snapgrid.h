// Snapshot grid (SnapGrid) as documented in docs/parallel.md
// Matt Young, 2022
#pragma once
#include <cstdint>
#include <cstring>

// Indexing: https://softwareengineering.stackexchange.com/a/212813

namespace ants {
    /// 2D snapshot grid, as documented in docs/parallel.md
    template<typename T>
    struct SnapGrid2D {
        /// Constructs a new empty SnapGrid
        explicit SnapGrid2D(int32_t size)  {
            this->size = size;
            clean = new T[size * size]{};
            dirty = new T[size * size]{};
        }

        SnapGrid2D() = default;

        // TODO also consider making read and write (or at least write) atomic(?)

        /// Writes a value into the dirty buffer
        inline constexpr void write(int32_t x, int32_t y, T value) {
            dirty[x + size * y] = value;
        }


        /// Reads a value from the snapshot grid, from the clean buffer
        inline constexpr T read(int32_t x, int32_t y) const {
            return clean[x + size * y];
        }

        /**
         * Commits the dirty buffer, i.e. replaces the current clean buffer with the current dirty
         * buffer.
         */
        inline constexpr void commit() {
            memcpy(clean, dirty, size * size * sizeof(T));
        }

        /// Clean buffer
        T *clean{};
        /// Dirty buffer
        T *dirty{};
        int32_t size{};
    };


    /// 3D snapshot grid, as documented in docs/parallel.md
    template<typename T>
    struct SnapGrid3D {
        /// Constructs a new empty SnapGrid
        explicit SnapGrid3D(int32_t size)  {
            this->size = size;
            clean = new T[size * size * size];
            dirty = new T[size * size * size];
        }

        SnapGrid3D() = default;

        // TODO also consider making read and write (or at least write) atomic(?)

        /// Writes a value into the dirty buffer
        inline constexpr void write(int32_t x, int32_t y, int32_t z, T value) {
            dirty[x + size * y + size * size * z] = value;
        }

        inline constexpr void write(int32_t x, int32_t y, uint32_t z, T value) {
            dirty[x + size * y + size * size * z] = value;
        }

        /**
         * Reads a value from the snapshot grid, from the clean buffer
         */
        inline constexpr T read(int32_t x, int32_t y, int32_t z) const {
            return clean[x + size * y + size * size * z];
        }

        inline constexpr T read(int32_t x, int32_t y, uint32_t z) const {
            return clean[x + size * y + size * size * z];
        }

        /**
         * Commits the dirty buffer, i.e. replaces the current clean buffer with the current dirty
         * buffer.
         */
        inline constexpr void commit() {
            memcpy(clean, dirty, size * size * size * sizeof(T));
        }

        /// Clean buffer
        T *clean{};
        /// Dirty buffer
        T *dirty{};
        int32_t size{};
    };
}