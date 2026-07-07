#pragma once

#include "CommonHeaders.h"

namespace mage::utl {

    /// <summary>
    /// A vector class similar to std::vector with basic functionality. <para/>
    /// User can specify in template argument whether they want elements' destructor to be called when being removed or while clearing/destructing the vector.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <typeparam name="destruct">Specify whether you want element's destructor to be called when being removed or while clearing/destructing the vector.</typeparam>
    template<typename T, bool destruct = true>
    class vector {
    public:
        vector() = default; // doesn't allocate any memory

        /// <summary>
        /// Constructor resizes the vector and initializes 'count' items.
        /// </summary>
        /// <param name="count"></param>
        constexpr explicit vector(u64 count) {
            resize(count);
        }

        /// <summary>
        /// Constructor resizes the vector and initializes 'count' items using 'value'.
        /// </summary>
        /// <param name="count"></param>
        /// <param name="value"></param>
        constexpr explicit vector(u64 count, const T& value) {
            resize(count, value);
        }

        template<typename it, typename = std::enable_if_t<std::_Is_iterator_v<it>>>
        constexpr explicit vector(it first, it last) {
            // when it is not an iterator type, enable_if results in template substitution failure,
            // which is not an error (SFINAE)
            for (; first != last; ++first) {
                emplace_back(*first);
            }
        }

        /// <summary>
        /// copy constructor. The items in the copied vector must be copyable.
        /// </summary>
        /// <param name="o"></param>
        constexpr vector(const vector& o) {
            *this = o;
        }

        /// <summary>
        /// move constructor. Constructs by moving another vector.
        /// <para/> The original vector will be empty after move.
        /// </summary>
        /// <param name="o"></param>
        constexpr vector(const vector&& o) 
            : _capacity{o._capacity}, _size{o._size}, _data{o._data}
        {
            o.reset();
        }

        /// <summary>
        /// Copy assignment operator. Clears this vector and copies items from another vector.
        /// <para/> The items must be copyable.
        /// </summary>
        /// <param name="o"></param>
        /// <returns></returns>
        constexpr vector& operator=(const vector& o) {
            assert(this != std::addressof(o)); // make sure vector is not assigned to itself.
            if (this != std::addressof(o)) {
                clear();
                reserve(o._size);
                for (auto& item : o) {
                    emplace_back(item);
                }
                assert(_size == o._size);
            }
            return *this;
        }

        /// <summary>
        /// Move assignment operator. 
        /// Frees all resources in this vector and moves the other vector into this one.
        /// </summary>
        /// <param name="o"></param>
        /// <returns></returns>
        constexpr vector& operator=(vector&& o) {
            assert(this != std::addressof(o)); // make sure vector is not assigned to itself.
            if (this != std::addressof(o)) {
                destroy();
                move(o);
            }
            return *this;
        }

        // destruct the vector and its items as specified in template's argument
        ~vector() { destroy(); }

        /// <summary>
        /// Inserts the item at the end of the vector by copying 'value'.
        /// </summary>
        /// <param name="value"></param>
        constexpr void push_back(const T& value) {
            emplace_back(value);
        }

        /// <summary>
        /// Inserts the item at the end of the vector by moving the 'value'.
        /// </summary>
        /// <param name="value"></param>
        constexpr void push_back(T&& value) {
            emplace_back(std::move(value));
        }

        /// <summary>
        /// Copy- or move-constructs an item at the end of the vector.
        /// </summary>
        /// <typeparam name="...params"></typeparam>
        /// <param name="...p"></param>
        /// <returns></returns>
        template<typename... params>
        constexpr decltype(auto) emplace_back(params&&... p) {
            if (_size == _capacity) {
                // reserve 50% more memory. (for large numbers ofc)
                // if _capacity=1 => (2*3)/2 = 3 => so +200% or 2x more
                // _capacity=100 -> (101*3)/2 = 151 => +51% more
                reserve(((_capacity + 1) * 3) >> 1); 
            }
            assert(_size < _capacity);

            // placement new to construct new instance of T at the end of array.
            T* const item = new (std::addressof(_data[_size])) T(std::forward<params>(p)...);
            ++_size;
            return *item;
        }

        /// <summary>
        /// Resizes the vector and initializes new items with their default value.
        /// </summary>
        /// <param name="new_size"></param>
        constexpr void resize(u64 new_size) {
            static_assert(std::is_default_constructible_v<T>, "Type must be default-constructable.");

            if (new_size > _size) {
                reserve(new_size);
                while (_size < new_size) {
                    emplace_back();
                }
            }
            else if (new_size < _size) {
                if constexpr (destruct) {
                    destruct_range(new_size, _size);
                    _size = new_size;
                }
            }

            // do nothing if new_size == size
            assert(new_size == _size);
        }

        /// <summary>
        /// Resizes the vector and initializes new items by copying 'value'
        /// </summary>
        /// <param name="new_size"></param>
        /// <param name="value"></param>
        constexpr void resize(u64 new_size, const T& value) {
            static_assert(std::is_copy_constructible_v<T>, "Type must be copy-constructable.");

            if (new_size > _size) {
                reserve(new_size);
                while (_size < new_size) {
                    emplace_back(value);
                }
            }
            else if (new_size < _size) {
                if constexpr (destruct) {
                    destruct_range(new_size, _size);
                }
                _size = new_size;
            }

            // do nothing if new_size == size
            assert(new_size == _size);
        }

        /// <summary>
        /// Allocates memory to contain the specified number of items.
        /// </summary>
        /// <param name="new_capacity"></param>
        constexpr void reserve(u64 new_capacity) {
            if (new_capacity > _capacity) {
                // if it's possible to expand the area of memory, realloc will do it,
                // therefore data will not be copied over to a new location, 
                // if region of memory cannot be expanded, realloc will allocate new region of memory
                // big enough and automatically copy our _data to new location.
                void* new_buffer = realloc(_data, new_capacity * sizeof(T));
                assert(new_buffer);
                if (new_buffer) {
                    _data = static_cast<T*>(new_buffer);
                    _capacity = new_capacity;
                }
            }
        }

        /// <summary>
        /// Removes the item at specified location.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        constexpr T* const erase(u64 index) {
            assert(_data && index < _size);
            return erase(std::addressof(_data[index]));
        }

        /// <summary>
        /// Removes the item at specified location.
        /// </summary>
        /// <param name="item"></param>
        /// <returns></returns>
        constexpr T* const erase(T* const item) {
            // we only copy data if the item was not the last item in the array.
            assert(_data && item >= std::addressof(_data[0]) && 
                   item < std::addressof(_data[_size]));
            if constexpr (destruct) item->~T();
            --_size;
            if (item < std::addressof(_data[_size])) {
                // put back memory from [item+1, _size] to where item is, so that we maintain order
                memcpy(item, item + 1, (std::addressof(_data[_size]) - item) * sizeof(T));
            }
            return item;
        }

        /// <summary>
        /// same as erase() but faster because it just copies last item.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        constexpr T* const erase_unordered(u64 index) {
            assert(_data && index < _size);
            return erase_unordered(std::addressof(_data[index]));
        }

        /// <summary>
        /// same as erase() but faster because it just copies last item.
        /// </summary>
        /// <param name="item"></param>
        /// <returns></returns>
        constexpr T* const erase_unordered(T* const item) {
            // we only copy data if the item was not the last item in the array.
            assert(_data && item >= std::addressof(_data[0]) &&
                   item < std::addressof(_data[_size]));
            if constexpr (destruct) item->~T();
            --_size;
            if (item < std::addressof(_data[_size])) {
                // copy last item from the array to where item was.
                memcpy(item, std::addressof(_data[_size]), sizeof(T));
            }
            return item;
        }

        /// <summary>
        /// Clears the vector and destructs items as specified in template argument.
        /// </summary>
        constexpr void clear() {
            if constexpr (destruct) {
                destruct_range(0, _size);
            }
            _size = 0;
        }

        /// <summary>
        /// swap 2 vectors
        /// </summary>
        /// <param name="o"></param>
        constexpr void swap(vector& o) {
            if (this != std::addressof(o)) {
                auto temp(std::move(o));
                o.move(*this);
                move(temp);
            }
        }

        /// <summary>
        /// Pointer to the start of data. Might be null.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr T* data() {
            return _data;
        }

        /// <summary>
        /// Pointer to the start of data. Might be null.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr T* const data() const {
            return _data;
        }

        // returns true if vector is empty.
        [[nodiscard]] constexpr bool empty() const {
            return _size == 0;
        }

        // returns size of vector (how many elements are in our buffer initialized)
        [[nodiscard]] constexpr u64 size() const {
            return _size;
        }

        // returns the current capacity of the vector (how much memory we have allocated for data storage)
        [[nodiscard]] constexpr u64 capacity() const {
            return _capacity;
        }

        // indexing operator. Returns reference to the item at specified index.
        [[nodiscard]] constexpr T& operator[](u64 index) {
            assert(_data && index < _size);
            return _data[index];
        }

        // indexing operator. Returns a constant reference to the item at specified index.
        [[nodiscard]] constexpr const T& operator[](u64 index) const {
            assert(_data && index < _size);
            return _data[index];
        }

        /// <summary>
        /// returns reference to the first item. Will fault the application if called when the vector is empty.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr T& front() {
            assert(_data && _size);
            return _data[0];
        }

        /// <summary>
        /// returns constant reference to the first item. Will fault the application if called when the vector is empty.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr const T& front() const {
            assert(_data && _size);
            return _data[0];
        }

        /// <summary>
        /// returns reference to the last item. Will fault the application if called when the vector is empty.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr T& back() {
            assert(_data && _size);
            return _data[_size - 1];
        }

        /// <summary>
        /// returns constant reference to the last item. Will fault the application if called when the vector is empty.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr const T& back() const {
            assert(_data && _size);
            return _data[_size - 1];
        }


        /// <summary>
       /// returns pointer to the first item. Returns null when vector is empty.
       /// </summary>
       /// <returns></returns>
        [[nodiscard]] constexpr T* begin() {
            assert(_data);
            return std::addressof(_data[0]);
        }

        /// <summary>
        /// returns constant pointer to the first item. Returns null when vector is empty.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr const T* begin() const {
            assert(_data);
            return std::addressof(_data[0]);
        }

        /// <summary>
        /// returns pointer to the last item. Returns null when vector is empty.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr T* end() {
            assert(_data);
            return std::addressof(_data[_size]);
        }

        /// <summary>
        /// returns constant pointer to the last item. Returns null when vector is empty.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr const T* end() const {
            assert(_data);
            return std::addressof(_data[_size]);
        }
    private:
        constexpr void move(vector& o) {
            _capacity = o._capacity;
            _size = o._size;
            _data = o._data;
            o.reset();
        }

        constexpr void reset() {
            _capacity = 0;
            _size = 0;
            _data = nullptr;
        }

        constexpr void destruct_range(u64 first, u64 last) {
            assert(destruct);
            assert(first <= _size && last <= _size && first <= last);
            if (_data) {
                for (; first != last; ++first) {
                    _data[first].~T();
                }
            }
        }

        constexpr void destroy() {
            // this assertion makes sure that if there's non-zero capacity, than we should have some _data.
            // and if our capacity is 0, than there shouldn't be any allocation made for _data pointer as well.
            assert([&] {return _capacity ? _data != nullptr : _data == nullptr; }());
            clear();
            _capacity = 0;
            if (_data) free(_data);
            _data = nullptr;
        }

    private:
        u64 _capacity   = 0;
        u64 _size       = 0;
        T*  _data       = nullptr;
    };
}