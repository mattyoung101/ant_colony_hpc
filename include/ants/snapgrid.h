// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
#pragma once
#include <cstdint>
#include <cstring>
#include "log/log.h"
#include "ants/defines.h"

// Snapshot grid (SnapGrid) as documented in docs/parallel.md

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
#if USE_MPI
            written = new bool[width * height]{};
#endif
        }

        SnapGrid2D() = default;

        /// Writes a value into the dirty buffer
        inline constexpr void write(int32_t x, int32_t y, T value) {
            dirty[x + width * y] = value;
#if USE_MPI
            written[x + width * y] = true;
#endif
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
#if USE_MPI
            memset(written, 0, width * height * sizeof(bool));
#endif
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
#if USE_MPI
        /// Positions in the dirty array that have been written since the last flush
        bool *written{};
#endif
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
            clean = new T[width * height * depth]{};
            dirty = new T[width * height * depth]{};
#if USE_MPI
            written = new bool[width * height * depth]{};
#endif
        }

        SnapGrid3D() = default;

        /// Writes a value into the dirty buffer
        template<class I>
        inline constexpr void write(int32_t x, int32_t y, I z, T value) {
            dirty[x + width * y + width * height * z] = value;
#if USE_MPI
            written[x + width * y + width * height * z] = true;
#endif
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
#if USE_MPI
            memset(written, 0, width * height * depth * sizeof(bool));
#endif
        }

        /// Clean buffer
        T *clean{};
        /// Dirty buffer
        T *dirty{};
#if USE_MPI
        /// Positions in the dirty array that have been written since the last flush
        bool *written{};
#endif
        int32_t width{}, height{}, depth{};
    };
}