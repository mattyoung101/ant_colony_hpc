// Snapshot grid (SnapGrid) as documented in docs/parallel.md
// Matt Young, 2022
#pragma once
#include <cstdint>
#include <cstring>

namespace ants {
    /// Snapshot grid, as documented in docs/parallel.md
    template<typename T>
    struct SnapGrid {
        /// Constructs a new empty SnapGrid
        explicit SnapGrid(int32_t size)  {
            this->size = size;
            clean = new T*[size];
            dirty = new T*[size];
        }

        SnapGrid() = default;

        /// Constructs a new SnapGrid from existing data
        SnapGrid(T **data, int32_t size) {
            this->size = size;
            clean = new T*[size];
            dirty = new T*[size];
            for (int i = 0; i < size; i++) {
                memcpy(clean[i], data[i], size * sizeof(T));
                memcpy(dirty[i], data[i], size * sizeof(T));
            }
        }

        /*~SnapGrid() {
            delete clean;
            delete dirty;
        }*/

        /**
         * Inserts a row into BOTH the dirty and clean grid. This should be used only for
         * initialisation.
         * @param y y index to insert the row at
         * @param row row data. Must be of length this->size
         */
        void insertRow(int32_t y, T *row) {
            clean[y] = row;
            dirty[y] = row;
        }

        // TODO also consider making read and write (or at least write) atomic

        /**
         * Writes a value into the dirty buffer
         * @param x x position
         * @param y y position
         * @param value value to insert
         */
        void write(int32_t x, int32_t y, T value) {
            dirty[y][x] = value;
        }

        /**
         * Reads a value from the snapshot grid, from the clean buffer
         * @param x x position
         * @param y y position
         * @return value of type T at this position in the clean buffer
         */
        T read(int32_t x, int32_t y) const {
            return clean[y][x];
        }

        /**
         * Commits the dirty buffer, i.e. replaces the current clean buffer with the current dirty
         * buffer.
         */
        void commit() {
            for (int i = 0; i < size; i++) {
                memcpy(clean[i], dirty[i], size * sizeof(T));
            }
        }


        /// Clean buffer
        T **clean{};
        /// Dirty buffer
        T **dirty{};
        int32_t size{};
    };
};