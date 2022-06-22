#ifndef __BLOCK_VECTOR_H__
#define __BLOCK_VECTOR_H__

#include <cstddef>
#include <array>
#include <utility>

#include <mem.h>

// A vector-like class that is made of a linked list of arrays. Sometimes known as an unrolled linked list.
// Each array holds `block_size` elements, which defaults to the maximum amount that fits in a single memory block.
// Allows for insertion and forward iteration. Movable, but not copyable.
template <typename T, size_t block_size = (mem_block_size - (sizeof(void*) + sizeof(size_t))) / sizeof(T)>
class block_vector
{
public:
    using value_type             = T;
    using size_type              = std::size_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    struct Block {
        Block *next;
        size_type count;
        // std::array<T, block_size> contents;
        std::aligned_storage<sizeof(T), alignof(T)>::type contents[block_size];


        // Block() = default;
        // Block(const Block&) = delete;
        // Block(Block&&) = delete;
        // Block& operator=(const Block&) = delete;
        // Block& operator=(Block&&) = delete;
        // ~Block()
        // {
        //     for (size_type i = 0; i < count; i++)
        //     {
        //         reinterpret_cast<T*>(&contents[i])->~T();
        //     }
        // }
    };
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        constexpr Iterator() : block_(nullptr), index_(0) {}
        constexpr Iterator(Block* block, size_type index) : block_(block), index_(index) {}
        constexpr reference operator*() const { return reinterpret_cast<reference>(block_->contents[index_]); }
        constexpr pointer operator->() { return reinterpret_cast<pointer>(&block_->contents[index_]); }
        constexpr Iterator& operator++()
        {
            ++index_;
            if (index_ >= block_size)
            {
                index_ = 0;
                block_ = block_->next;
            }
            return *this;
        }
        constexpr Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++index_;
            if (index_ >= block_size)
            {
                index_ = 0;
                block_ = block_->next;
            }
            return tmp;
        }

        constexpr friend bool operator== (const Iterator& a, const Iterator& b) { return a.block_ == b.block_ && a.index_ == b.index_; }
        constexpr friend bool operator!= (const Iterator& a, const Iterator& b) { return a.block_ != b.block_ || a.index_ != b.index_; }
        Block *block_;
        size_type index_;
    };
    struct ConstIterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        constexpr ConstIterator(Block* block, size_type index) : block_(block), index_(index) {}
        constexpr reference operator*() const { return block_.contents[index_]; }
        constexpr pointer operator->() { return &block_.contents[index_]; }
        constexpr ConstIterator& operator++()
        {
            ++index_;
            if (index_ >= block_size)
            {
                index_ = 0;
                block_ = block_->next;
            }
            return *this;
        }
        constexpr ConstIterator operator++(int)
        {
            ConstIterator tmp = *this;
            ++index_;
            if (index_ >= block_size)
            {
                index_ = 0;
                block_ = block_->next;
            }
            return tmp;
        }
        constexpr friend bool operator== (const ConstIterator& a, const ConstIterator& b) { return a.block_ == b.block_ && a.index_ == b.index_; }
        constexpr friend bool operator!= (const ConstIterator& a, const ConstIterator& b) { return a.block_ != b.block_ || a.index_ != b.index_; }

        constexpr friend bool operator== (const ConstIterator& a, const Iterator& b) { return a.block_ == b.block_ && a.index_ == b.index_; }
        constexpr friend bool operator!= (const ConstIterator& a, const Iterator& b) { return a.block_ != b.block_ || a.index_ != b.index_; }

        constexpr friend bool operator== (const Iterator& a, const ConstIterator& b) { return a.block_ == b.block_ && a.index_ == b.index_; }
        constexpr friend bool operator!= (const Iterator& a, const ConstIterator& b) { return a.block_ != b.block_ || a.index_ != b.index_; }
        Block *block_;
        size_type index_;
    };
    using iterator               = Iterator;
    using const_iterator         = ConstIterator;

    // Default constructor
    block_vector() : first_(new Block), last_(first_)
    {
        first_->next = nullptr;
        first_->count = 0;
    }
    // Destructor
    ~block_vector()
    {
        free_chain(first_);
        first_ = last_ = nullptr;
    }
    // Copy constructor (not copyable)
    block_vector(const block_vector&) = delete;
    // Copy assignment (not copyable)
    block_vector& operator=(const block_vector&) = delete;
    // Move constructor
    block_vector(block_vector&& rhs) : first_(std::exchange(rhs.first_, nullptr)), last_(std::exchange(rhs.last_, nullptr))
    {}
    // Move assignment
    block_vector& operator=(block_vector&& rhs)
    {
        free_chain(first_);
        first_ = std::exchange(rhs.first_, nullptr);
        last_ = std::exchange(rhs.last_, nullptr);
        return *this;
    }

    template <typename... Args>
    __attribute__((noinline)) constexpr iterator emplace_back(Args&&... args) noexcept
    {
        new (&last_->contents[last_->count]) T(std::forward<Args>(args)...);
        iterator ret{last_, last_->count++};
        if (last_->count == block_size)
        {
            add_block();
        }
        return ret;
    }

    constexpr iterator push_back(const T& val) noexcept
    {
        new (&last_->contents[last_->count]) T(val);
        iterator ret{last_, last_->count++};
        if (last_->count == block_size)
        {
            add_block();
        }
        return ret;
    }

    const_iterator begin() const noexcept { return {first_, 0}; }
    iterator       begin()       noexcept { return {first_, 0}; }

    const_iterator end() const noexcept { return {last_, last_->count}; }
    iterator       end()       noexcept { return {last_, last_->count}; }

    const_iterator back() const noexcept { return {last_, last_->count - 1}; }
    iterator       back()       noexcept { return {last_, last_->count - 1}; }

    struct BlockIterator
    {
        constexpr BlockIterator(Block* block) : block_(block) {}
        constexpr Block& operator*() const { return *block_; }
        constexpr Block* operator->() { return block_; }
        constexpr BlockIterator& operator++()
        {
            block_ = block_->next;
            return *this;
        }
        constexpr BlockIterator operator++(int)
        {
            BlockIterator tmp = *this;
            block_ = block_->next;
            return tmp;
        }
        friend constexpr bool operator== (const BlockIterator& a, const BlockIterator& b) { return a.block_ == b.block_; }
        friend constexpr bool operator!= (const BlockIterator& a, const BlockIterator& b) { return a.block_ != b.block_; }
    private:
        Block* block_;
    };

    BlockIterator blocks_begin() const noexcept { return {first_}; }
    BlockIterator blocks_end()   const noexcept { return {nullptr}; }

    bool empty() const noexcept { return first_ == nullptr || first_->count == 0; }
private:
    void add_block()
    {
        Block* new_block = new Block;
        last_->next = new_block;
        last_ = new_block;
        new_block->next = nullptr;
        new_block->count = 0;
    }
    void free_chain(Block *start)
    {
        Block *cur_block = start;
        while (cur_block != nullptr)
        {
            Block *next = cur_block->next;
            delete cur_block;
            cur_block = next;
        }
    }
    Block* first_;
    Block* last_;
};


#endif