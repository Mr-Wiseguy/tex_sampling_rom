#ifndef __DYNAMIC_ARRAY_H__
#define __DYNAMIC_ARRAY_H__

#include <cstddef>
#include <memory>
#include <type_traits>
#include <initializer_list>

template <typename T, typename ...Ts>
inline constexpr bool areT_v = std::conjunction_v<std::is_same<T,Ts>...>;

template <typename T, typename ...Ts>
inline constexpr bool areConvertibleT_v = std::conjunction_v<std::is_convertible<Ts,T>...>;

template <typename T>
std::unique_ptr<T> make_unique_uninitialized(size_t size) {
    return std::unique_ptr<T>(new typename std::remove_extent<T>::type[size]);
}

template <typename T>
class dynamic_array {
public:
    using value_type             = T;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = value_type*;
    using const_iterator         = const value_type*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using storage_type           = std::aligned_storage<sizeof(T), alignof(T)>::type;

    dynamic_array() :
        data_(nullptr),
        size_(0)
    {

    }

    explicit dynamic_array(size_type size) :
        data_(std::unique_ptr<storage_type[]>(new storage_type[size]())),
        size_(size)
    {
        std::uninitialized_default_construct_n(begin(), size);
    }

    explicit dynamic_array(size_type size, const T& value) :
        data_(std::unique_ptr<storage_type[]>(new storage_type[size])),
        size_(size)
    {
        std::uninitialized_fill_n(begin(), size, value);
    }

    dynamic_array(std::initializer_list<T> list) :
        data_(std::unique_ptr<storage_type[]>(new storage_type[list.size()])),
        size_(list.size())
    {
        std::uninitialized_copy_n(list.begin(), size_, begin());
    }

    // Destructor
    ~dynamic_array()
    {
        // Destroy objects in data_
        std::destroy_n(begin(), size_);
        // data_ unique_ptr will get deallocated automatically
    }

    // Copy constructor
    dynamic_array(const dynamic_array& rhs) :
        data_(std::unique_ptr<storage_type[]>(new storage_type[rhs.size_])),
        size_(rhs.size_)
    {
        std::uninitialized_copy_n(rhs.begin(), size_, begin());
    }
    
    // Move constructor
    dynamic_array(dynamic_array&& other) noexcept :
        data_(std::move(other.data_)),
        size_(std::exchange(other.size_, 0)) { }
    
    // Copy assignment
    dynamic_array& operator=(const dynamic_array& rhs)
    {
        // Prevent copying this object to itself
        if (this == &rhs) return *this;
        // Destroy any objects in this object's existing data
        std::destroy_n(begin(), size_);
        // Allocate new data
        data_ = std::unique_ptr<storage_type[]>(new storage_type[rhs.size_]);
        // Copy the input data into the newly allocated data
        size_ = rhs.size_;
        std::uninitialized_copy_n(rhs.begin(), size_, begin());
        return *this;
    }

    // Move assignment
    dynamic_array& operator=(dynamic_array&& rhs) noexcept
    {
        // Destroy any objects in this object's existing data
        std::destroy_n(begin(), size_);
        // Move the rhs object's data into this object
        data_ = std::move(rhs.data_);
        // Set this object's size to the rhs object's size, and zero the rhs object's size
        size_ = std::exchange(rhs.size_, 0);
        return *this;
    };

    const T& operator[](size_type index) const noexcept { return reinterpret_cast<reference>(data_[index]); }
    T&       operator[](size_type index)       noexcept { return reinterpret_cast<reference>(data_[index]); }
    
    const T* data() const noexcept { return reinterpret_cast<pointer>(data_.get()); }
    T*       data()       noexcept { return reinterpret_cast<pointer>(data_.get()); }

    const_iterator begin() const noexcept { return reinterpret_cast<pointer>(&data_[0]); }
    iterator       begin()       noexcept { return reinterpret_cast<pointer>(&data_[0]); }

    const_iterator end() const noexcept { return reinterpret_cast<pointer>(&data_[size_]); }
    iterator       end()       noexcept { return reinterpret_cast<pointer>(&data_[size_]); }

    const_reverse_iterator rbegin() const noexcept { return reinterpret_cast<pointer>(&data_[size_ - 1]); }
    reverse_iterator       rbegin()       noexcept { return reinterpret_cast<pointer>(&data_[size_ - 1]); }

    const_reverse_iterator rend() const noexcept { return reinterpret_cast<pointer>(&data_[-1]); }
    reverse_iterator       rend()       noexcept { return reinterpret_cast<pointer>(&data_[-1]); }

    T* release() noexcept { size_ = 0; return reinterpret_cast<pointer>(data_.release()); }

    void truncate(size_type new_size) { if (new_size < size_) { size_ = new_size; }}

    size_type size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
private:
    std::unique_ptr<storage_type[]> data_;
    size_type size_;
};

#endif
