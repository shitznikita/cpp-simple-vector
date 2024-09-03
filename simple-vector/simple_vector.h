#pragma once

#include "array_ptr.h"

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std::literals::string_literals;

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity_to_reserve) : capacity_to_reserve_(capacity_to_reserve) {}

    size_t GetCapacity() const noexcept {
        return capacity_to_reserve_;
    }

private:
    size_t capacity_to_reserve_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size) : items_(ArrayPtr<Type>(size)),
                                         size_(size), 
                                         capacity_(size) 
    {
        std::fill(begin(), end(), Type());
    }

    SimpleVector(size_t size, const Type& value) : items_(ArrayPtr<Type>(size)), 
                                                   size_(size), 
                                                   capacity_(size) 
    {
        std::fill(begin(), end(), value);
    }

    SimpleVector(std::initializer_list<Type> init) : items_(ArrayPtr<Type>(init.size())), 
                                                     size_(init.size()), 
                                                     capacity_(init.size()) 
    {
        std::copy(init.begin(), init.end(), begin());
    }

    SimpleVector(const SimpleVector& other) : SimpleVector()
    {
        Copy(other);
    }

    SimpleVector(const ReserveProxyObj& obj) : SimpleVector() 
    {
        Reserve(obj.GetCapacity());
    }

    SimpleVector(SimpleVector&& other) : items_(std::move(other.items_)), 
                                         size_(std::exchange(other.size_, 0)), 
                                         capacity_(std::exchange(other.capacity_, 0))
    {
    }

    Type& operator[](size_t index) noexcept {
        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return items_[index];
    }

    SimpleVector& operator=(const SimpleVector& other) {
        if (this != &other) {
            Copy(other);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& other) {
        if (this != &other) {
            items_ = std::move(other.items_);
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
        }
        return *this;
    }

    Iterator begin() noexcept {
        return Iterator(items_.Get());
    }
    Iterator end() noexcept {
        return Iterator(items_.Get() + size_);
    }
    ConstIterator begin() const noexcept {
        return ConstIterator(items_.Get());
    }
    ConstIterator end() const noexcept {
        return ConstIterator(items_.Get() + size_);
    }
    ConstIterator cbegin() const noexcept {
        return ConstIterator(items_.Get());
    }
    ConstIterator cend() const noexcept {
        return ConstIterator(items_.Get() + size_);
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return (size_ == 0);
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("The index is outside the array"s);
        }
        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("The index is outside the array"s);
        }
        return items_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
        } else if (new_size <= capacity_) {
            Fill(begin() + size_, begin() + new_size);
            size_ = new_size; 
        } else {
            size_t new_capacity = std::max(new_size, capacity_ * 2);
            ArrayPtr<Type> tmp(new_capacity);
            std::move(begin(), end(), tmp.Get());
            Fill(tmp.Get() + size_, tmp.Get() + new_size);
            items_.swap(tmp);
            size_ = new_size;
            capacity_ = new_capacity;
        }
    }

    void PushBack(const Type& item) {
        if (size_ < capacity_) {
            items_[size_] = item;
            ++size_;
        } else {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> tmp(new_capacity);
            std::copy(begin(), end(), tmp.Get());
            tmp[size_] = item;
            items_.swap(tmp);
            ++size_;
            capacity_ = new_capacity;
        }
    }

    void PushBack(Type&& item) {
        if (size_ < capacity_) {
            items_[size_] = std::move(item);
            ++size_;
        } else {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> tmp(new_capacity);
            std::move(begin(), end(), tmp.Get());
            tmp[size_] = std::move(item);
            items_.swap(tmp);
            ++size_;
            capacity_ = new_capacity;
        }
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        auto dist = std::distance(begin(), Iterator(pos));
        if (size_ < capacity_) {
            std::copy_backward(Iterator(pos), end(), end() + 1);
            *(begin() + dist) = value;
            ++size_;
        } else {
            size_t new_capacity = capacity_ == 0 ? 1: capacity_ * 2;
            ArrayPtr<Type> tmp(new_capacity);
            std::copy(begin(), begin() + dist, tmp.Get());
            *(tmp.Get() + dist) = value;
            std::copy(begin() + dist, end(), tmp.Get() + dist + 1);
            items_.swap(tmp);
            ++size_;
            capacity_ = new_capacity;
        }
        return begin() + dist;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        auto dist = std::distance(begin(), Iterator(pos));
        if (size_ < capacity_) {
            std::move(Iterator(pos), end(), Iterator(pos) + 1);
            *(begin() + dist) = std::move(value);
            ++size_;
        } else {
            size_t new_capacity = capacity_ == 0 ? 1: capacity_ * 2;
            ArrayPtr<Type> tmp(new_capacity);
            std::move(begin(), begin() + dist, tmp.Get());
            *(tmp.Get() + dist) = std::move(value);
            std::move(begin() + dist, end(), tmp.Get() + dist + 1);
            items_.swap(tmp);
            ++size_;
            capacity_ = new_capacity;
        }
        return begin() + dist;
    }

    void PopBack() noexcept {
        --size_;
    }

    Iterator Erase(ConstIterator pos) {
        auto first = Iterator(pos);
        auto second = first + 1;
        while (second != end()) {
            *first = std::move(*second);
            ++first;
            ++second;
        }
        --size_;
        return Iterator(pos);
    }

    void Reserve(size_t new_capacity) {
        if (capacity_ < new_capacity) {
            ArrayPtr<Type> tmp(new_capacity);
            std::move(begin(), end(), tmp.Get());
            items_.swap(tmp);
            capacity_  = new_capacity;
        }
    }

    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void Copy(const SimpleVector& other) {
        SimpleVector tmp(other.size_);
        auto tmp_it = tmp.begin();
        for (auto it = other.begin(); it != other.end(); ++it, ++tmp_it) {
            *tmp_it = *it;
        }
        tmp.size_ = other.size_;
        tmp.capacity_ = other.capacity_;
        swap(tmp);
    }

    void Fill(Iterator begin, Iterator end) {
        for (auto it = begin; it != end; ++it) {
            *it = Type();
        }
    }

private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (&lhs == &rhs) {
        return true;
    } else if (lhs.GetSize() != rhs.GetSize()) {
        return false;
    }
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs > lhs);
} 