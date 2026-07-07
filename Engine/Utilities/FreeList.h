#pragma once
#include "CommonHeaders.h"

#if USE_STL_VECTOR
#pragma message("WARNING: using utl::free_list with std::vector results in duplicate calls to class constructor!")
#endif

namespace mage::utl {
    template<typename T>
    class free_list {
        static_assert(sizeof(T) >= sizeof(u32)); // data type must be at least 4 bytes of size
    public:
        free_list() = default;
        explicit free_list(u32 count) {
            _array.reserve(count);
        }
        ~free_list() {
            assert(!_size); // all slots have been freed (or not if it fails)
#if USE_STL_VECTOR
            memset(_array.data(), 0, _array.size() * sizeof(T));
#endif
        }

        /// <summary>
        /// Returns index at which new item has been added.
        /// </summary>
        /// <typeparam name="...params"></typeparam>
        /// <param name="...p">Parameters for constructor of the type you have in this list.</param>
        /// <returns></returns>
        template<class... params>
        constexpr u32 add(params&&... p) {
            u32 id = u32_invalid_id;
            if (_next_free_index == u32_invalid_id) {
                // means we have no available free slots and we need to allocate a new one, 
                // can do that by emplacing_back to out underlying vector
                id = (u32)_array.size();
                _array.emplace_back(std::forward<params>(p)...);
            }
            else {
                id = _next_free_index;
                assert(id < _array.size() && already_removed(id));

                // using std::addressof() in case type T overrides & operator
                _next_free_index = *(const u32* const)std::addressof(_array[id]);
                // using placement new operator to construct an instance of T (using constructor parameters that it might have)
                // at the address we want it to be. (so that we have continuity of memory blocks/chunks)
                new (std::addressof(_array[id])) T(std::forward<params>(p)...);
            }
            ++_size;
            return id; // return index of new item
        }

        constexpr void remove(u32 id) {
            assert(id < _array.size() && !already_removed(id));

            T& item = _array[id];
            item.~T(); // destruct  the item
            // indicate the slot has been removed by filling it with value 0f 0xcc (arbitrary value, you can choose smth else,
            // but according to this https://stackoverflow.com/questions/370195/when-and-why-will-a-compiler-initialise-memory-to-0xcd-0xdd-etc-on-malloc-fre
            // uninitialized variables are automatically assigned value of 0xCC)
            // only in debug build, this expression is nothing in release builds.
            DEBUG_ONLY_EXPR(memset(std::addressof(_array[id]), 0xCC, sizeof(T)));
            // write the index of next free slot in first 4 bytes of this location
            *(u32* const)std::addressof(_array[id]) = _next_free_index;
            _next_free_index = id;
            --_size;
        }

        constexpr u32 size() const {
            return _size;
        }

        constexpr u32 capacity() const {
            return _array.size();
        }

        constexpr bool empty() const {
            return _size == 0;
        }

        [[nodiscard]] constexpr T& operator[](u32 id) {
            assert(id < _array.size() && !already_removed(id));
            return _array[id];
        }

        [[nodiscard]] constexpr const T& operator[](u32 id) const {
            assert(id < _array.size() && !already_removed(id));
            return _array[id];
        }

    private:
        constexpr bool already_removed(u32 id) {
            // NOTE: when sizeof(T) == sizeof(u32), we can't test if the item was already removed.
            if constexpr (sizeof(T) > sizeof(u32)) {
                u32 i = sizeof(u32); // skip first 4 bytes
                // convert to array of bytes
                const u8* const p = (const u8* const)std::addressof(_array[id]);
                while ((p[i] == 0xCC) && (i < sizeof(T))) ++i;
                // if in our array of bytes(representing our memory chunk/slot for an item, only contains 0xCC for all bytes, 
                // then item has been removed (and we return true)
                // NOTE: of course this will work only if type T doesn't happen to only contain  an array of 0xCC bytes as 
                //       it's valid data.
                return i == sizeof(T);
            }
            else {
                return true;
            }
        }
    private:
#if USE_STL_VECTOR
        utl::vector<T>              _array;
#else
        utl::vector<T, false>       _array; // underlying buffer, false template param moves responsibility of destructing 
                                            // objects in it to the which is done in free_list::remove() by calling ~T() on item 
#endif
        u32                         _next_free_index = u32_invalid_id;
        u32                         _size = 0;
    };
}