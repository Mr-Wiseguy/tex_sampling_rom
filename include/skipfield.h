#ifndef __SKIPFIELD_H__
#define __SKIPFIELD_H__

#include <iterator>
#include <array>
#include <initializer_list>

// Stores a sequence of elements that can be inserted to and removed from without invalidating element pointers or
// moving elements around. Elements are stored in an underlying array that can have gaps, which are automatically
// skipped by iterators. Insertion places the new element into the first free space in the array.
// Stores an array of bytes that's used to determine if a given slot in the underlying array is free or not.
// A value of 0 indicates the slot is occupied, a value of 0xFF indicates that the slot is free.
// 0xFF is used to indicate free slots because it allows the compiler to load -1 into a register and write to multiple
// slots at once (the full register width) when clearing the skipfield, as opposed to 1 which would incur a memset instead.
template <typename T, size_t Length>
class skipfield {
public:
    struct Iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        constexpr Iterator(pointer ptr, uint8_t* skipped) : ptr_(ptr), skipped_(skipped) {}
        constexpr reference operator*() const { return *ptr_; }
        constexpr pointer operator->() { return ptr_; }
        constexpr Iterator& operator++()
        {
            do
            {
                ptr_++;
                skipped_++;
            } while (*skipped_);
            return *this;
        }
        constexpr Iterator& operator--()
        {
            do
            {
                ptr_--;
                skipped_--;
            } while (*skipped_);
            return *this;
        }
        constexpr Iterator operator++(int)
        {
            Iterator tmp = *this;
            do
            {
                ptr_++;
                skipped_++;
            } while (*skipped_);
            return tmp;
        }
        constexpr Iterator operator--(int)
        {
            Iterator tmp = *this;
            do
            {
                ptr_--;
                skipped_--;
            } while (*skipped_);
            return tmp;
        }

        constexpr friend bool operator== (const Iterator& a, const Iterator& b) { return a.skipped_ == b.skipped_; }
        constexpr friend bool operator!= (const Iterator& a, const Iterator& b) { return a.skipped_ != b.skipped_; }
        pointer ptr_;
        uint8_t* skipped_;
    };
    struct ConstIterator {
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = const T;
        using pointer           = const T*;
        using reference         = const T&;

        constexpr ConstIterator(pointer ptr, const uint8_t* skipped) : ptr_(ptr), skipped_(skipped) {}
        constexpr ConstIterator(Iterator iter) : ptr_(iter.ptr_), skipped_(iter.skipped_) {}
        constexpr reference operator*() const { return *ptr_; }
        constexpr pointer operator->() { return ptr_; }
        constexpr ConstIterator& operator++()
        {
            do
            {
                ptr_++;
                skipped_++;
            } while (*skipped_);
            return *this;
        }
        constexpr ConstIterator& operator--()
        {
            do
            {
                ptr_--;
                skipped_--;
            } while (*skipped_);
            return *this;
        }
        constexpr ConstIterator operator++(int)
        {
            ConstIterator tmp = *this;
            do
            {
                ptr_++;
                skipped_++;
            } while (*skipped_);
            return tmp;
        }
        constexpr ConstIterator operator--(int)
        {
            ConstIterator tmp = *this;
            do
            {
                ptr_--;
                skipped_--;
            } while (*skipped_);
            return tmp;
        }

        constexpr friend bool operator== (const ConstIterator& a, const ConstIterator& b) { return a.skipped_ == b.skipped_; }
        constexpr friend bool operator!= (const ConstIterator& a, const ConstIterator& b) { return a.skipped_ != b.skipped_; }

        constexpr friend bool operator== (const ConstIterator& a, const Iterator& b) { return a.skipped_ == b.skipped_; }
        constexpr friend bool operator!= (const ConstIterator& a, const Iterator& b) { return a.skipped_ != b.skipped_; }

        constexpr friend bool operator== (const Iterator& a, const ConstIterator& b) { return a.skipped_ == b.skipped_; }
        constexpr friend bool operator!= (const Iterator& a, const ConstIterator& b) { return a.skipped_ != b.skipped_; }
        pointer ptr_;
        const uint8_t* skipped_;
    };
    using value_type             = T;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = Iterator;
    using const_iterator         = ConstIterator;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using storage_type           = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

    constexpr skipfield() :
        data_{},
        skipped_{},
        num_items_(0)
    {
        skipped_.fill(0xFF);
        skipped_.back() = 0;
    }


    constexpr explicit skipfield(size_type size) :
        data_{},
        skipped_{},
        num_items_(size)
    {
        auto stop = skipped_.begin() + size;
        std::fill(skipped_.begin(), stop, 0);
        std::fill(stop, skipped_.end(), 0xFF);
        skipped_.back() = 0;
    }

    constexpr explicit skipfield(size_type size, const T& value) :
        data_{},
        skipped_{},
        num_items_(size)
    {
        auto stop = skipped_.begin() + size;
        std::fill(skipped_.begin(), stop, 0);
        std::fill(stop, skipped_.end(), 0xFF);
        std::uninitialized_fill_n(reinterpret_cast<pointer>(data_.begin()), size, value);
        skipped_.back() = 0;
    }

    constexpr skipfield(std::initializer_list<T> list) :
        data_{},
        skipped_{},
        num_items_(list.size())
    {
        auto stop = skipped_.begin() + num_items_;
        std::fill(skipped_.begin(), stop, 0);
        std::fill(stop, skipped_.end(), 0xFF);
        std::uninitialized_copy(list.begin(), list.end(), reinterpret_cast<pointer>(data_.begin()));
        skipped_.back() = 0;
    }

    // Destructor
    ~skipfield() {
        for (size_t index = 0; index < Length; index++) {
            if (!skipped_[index]) {
                std::destroy_at(reinterpret_cast<pointer>(&data_[index]));
            }
        }
    }

    // Copy constructor
    constexpr skipfield(const skipfield& other) :
        skipped_{other.skipped_}
    {
        for (size_t idx = 0; idx < Length; idx++) {
            if (!skipped_[idx]) {
                std::uninitialized_copy_n(reinterpret_cast<pointer>(other.data_.begin() + idx), 1, reinterpret_cast<pointer>(data_.begin() + idx));
            }
        }
        num_items_ = other.num_items_;
    }
    
    // Move constructor
    constexpr skipfield(skipfield&& other) noexcept
    {
        for (size_t idx = 0; idx < Length; idx++) {
            skipped_[idx] = std::exchange(other.skipped_[idx], true);
            if (!skipped_[idx]) {
                std::uninitialized_move_n(reinterpret_cast<pointer>(other.data_.begin() + idx), 1, reinterpret_cast<pointer>(data_.begin() + idx));
            }
        }
        num_items_ = std::exchange(other.num_items_, 0);
    }
    
    // Copy assignment
    constexpr skipfield& operator=(const skipfield& other)
    {
        if (this == &other) return *this;
        for (size_t idx = 0; idx < Length; idx++) {
            uint8_t this_skipped = skipped_[idx];
            uint8_t other_skipped = other.skipped_[idx];
            if (!this_skipped && !other_skipped) {
                // Both dst and src have data, so move it as usual
                std::copy_n(reinterpret_cast<pointer>(other.data_.begin() + idx), 1, reinterpret_cast<pointer>(data_.begin() + idx));
            } else if (!this_skipped && other_skipped) {
                // Src has no data, destroy the dst
                std::destroy_at(reinterpret_cast<pointer>(other.data_.begin() + idx));
            } else if (this_skipped && !other_skipped) {
                // Dst has no data, move the src into uninitialized data
                std::uninitialized_copy_n(reinterpret_cast<pointer>(other.data_.begin() + idx), 1, reinterpret_cast<pointer>(data_.begin() + idx));
            }
            skipped_[idx] = other_skipped;
        }
        num_items_ = other.num_items_;
        return *this;
    }

    // Move assignment
    constexpr skipfield& operator=(skipfield&& other) noexcept
    {
        if (this == &other) return *this;
        for (size_t idx = 0; idx < Length; idx++) {
            uint8_t this_skipped = skipped_[idx];
            uint8_t other_skipped = other.skipped_[idx];
            if (!this_skipped && !other_skipped) {
                // Both dst and src have data, so move it as usual
                *reinterpret_cast<pointer>(other.data_.begin() + idx) = std::move(*reinterpret_cast<pointer>(data_.begin() + idx));
            } else if (!this_skipped && other_skipped) {
                // Src has no data, destroy the dst
                std::destroy_at(reinterpret_cast<pointer>(other.data_.begin() + idx));
            } else if (this_skipped && !other_skipped) {
                // Dst has no data, move the src into uninitialized data
                std::uninitialized_move_n(reinterpret_cast<pointer>(other.data_.begin() + idx), 1, reinterpret_cast<pointer>(data_.begin() + idx));
            }
            skipped_[idx] = std::exchange(other.skipped_[idx], true);
        }
        num_items_ = std::exchange(other.num_items_, 0);
        return *this;
    }

    constexpr const_iterator begin() const noexcept { size_type i = first_idx(); return const_iterator{reinterpret_cast<pointer>(&data_[i]), &skipped_[i]}; }
    constexpr iterator       begin()       noexcept { size_type i = first_idx(); return iterator{reinterpret_cast<pointer>(&data_[i]), &skipped_[i]}; }

    constexpr const_iterator end() const noexcept { return const_iterator{nullptr, &skipped_[Length]}; }
    constexpr iterator       end()       noexcept { return iterator{nullptr, &skipped_[Length]}; }
    
    constexpr const_reverse_iterator rbegin() const noexcept { size_type i = last_idx(); return std::make_reverse_iterator(const_iterator{reinterpret_cast<pointer>(&data_[i + 1]), &skipped_[i + 1]}); }
    constexpr reverse_iterator       rbegin()       noexcept { size_type i = last_idx(); return std::make_reverse_iterator(iterator{reinterpret_cast<pointer>(&data_[i + 1]), &skipped_[i + 1]}); }

    constexpr const_reverse_iterator rend() const noexcept { return std::make_reverse_iterator(const_iterator{nullptr, skipped_.begin()}); }
    constexpr reverse_iterator       rend()       noexcept { return std::make_reverse_iterator(iterator{nullptr, skipped_.begin()}); }

    constexpr iterator erase(const_iterator pos) noexcept
    {
        iterator ret{const_cast<pointer>(pos.ptr_), const_cast<uint8_t*>(pos.skipped_)};
        *ret.skipped_ = 0xFF;
        ret.ptr_->~T();
        num_items_--;
        return ++ret;
    }

    // Erases the element at the given address
    constexpr void erase(value_type* element) noexcept
    {
        size_t index = (element - reinterpret_cast<pointer>(data_.data()));
        if (skipped_[index] == 0)
        {
            skipped_[index] = 0xFF;
            element->~T();
            num_items_--;
        }
    }
    
    constexpr iterator insert(const value_type& value) noexcept
    {
        if (num_items_ == Length)
        {
            return end();
        }
        size_t idx = first_free();
        skipped_[idx] = 0;
        new (reinterpret_cast<pointer>(&data_[idx])) T{value};
        num_items_++;
        return iterator{reinterpret_cast<pointer>(&data_[idx]), &skipped_[idx]};
    }
    
    template <typename... Args>
    __attribute__((noinline)) constexpr iterator emplace(Args&&... args) noexcept
    {
        if (num_items_ == Length)
        {
            return end();
        }
        size_t idx = first_free();
        skipped_[idx] = 0;
        new (reinterpret_cast<pointer>(&data_[idx])) T{std::forward<Args>(args)...};
        num_items_++;
        return iterator{reinterpret_cast<pointer>(&data_[idx]), &skipped_[idx]};
    }

    constexpr size_type size() const noexcept { return num_items_; }
    constexpr bool empty() const noexcept { return num_items_ == 0; }
    constexpr bool full() const noexcept { return num_items_ == Length; }
    constexpr void clear() noexcept { num_items_ = 0; std::fill_n(skipped_.begin(), Length, 0xFF); }
public:
    std::array<storage_type, Length> data_;
    std::array<uint8_t, Length + 1> skipped_;
    size_type num_items_;

    constexpr __attribute__((noinline)) size_type first_idx() const
    {
        size_type i;
        for (i = 0; skipped_[i] && i < Length; i++);
        return i;
    }

    constexpr size_type last_idx() const
    {
        size_type i;
        for (i = Length - 1; skipped_[i] && i != static_cast<size_type>(-1); i--);
        return i;
    }

    constexpr size_type first_free() const
    {
        size_type i;
        for (i = 0; !skipped_[i] && i < Length; i++);
        return i;
    }

    constexpr size_type last_free() const
    {
        size_type i;
        for (i = Length - 1; !skipped_[i] && i != static_cast<size_type>(-1); i--);
        return i;
    }
};

#endif
