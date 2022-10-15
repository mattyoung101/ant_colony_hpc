// Snapshot grid (SnapGrid) as documented in docs/parallel.md
// Matt Young, 2022
#include <cstdint>
#include <cstring>

namespace ants {
    template<typename T>
    /// Snapshot grid, as documented in docs/parallel.md
    struct SnapGrid {
        explicit SnapGrid(int32_t size) {
            this->size = size;
            clean = new T[size];
            dirty = new T[size];
        }

        SnapGrid(T *data, int32_t size) {
            clean = new T[size];
            dirty = new T[size];
            memcpy(clean, data, size * sizeof(T));
            memcpy(dirty, data, size * sizeof(T));
            this->size = size;
        }

        ~SnapGrid() {
            delete clean;
            delete dirty;
        }

        /**
         * Writes a value into the dirty buffer
         * @param x
         * @param y
         * @param value
         */
        void write(int32_t x, int32_t y, T value);

        /**
         * Reads a value from the snapshot grid, from the clean buffer
         * @param x x position
         * @param y y position
         * @return value of type T at this position in the clean buffer
         */
        T read(int32_t x, int32_t y);

        /**
         * Commits the dirty buffer, i.e. replaces the current clean buffer with the current dirty
         * buffer.
         */
        void commit();

        /// Clean buffer
        T *clean;
        /// Dirty buffer
        T *dirty;
        int32_t size{};
    };
};