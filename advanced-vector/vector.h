#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <exception>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    
    RawMemory(RawMemory&& other) noexcept
        : capacity_(std::move(other.capacity_)) {

        buffer_ = std::move(other.buffer_);
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        Swap(rhs);
       
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // ����������� �������� ����� ������ ������, ��������� �� ��������� ��������� �������
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // �������� ����� ������ ��� n ��������� � ���������� ��������� �� ��
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // ����������� ����� ������, ���������� ����� �� ������ buf ��� ������ Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other)
    {
        Swap(other);
    }
    
    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                /* ����������� �������� �� rhs, ������ ��� ������������� �����
                   ��� ������ ������������ */
                /* 1. ������ �������-��������� ������ ������� �������-��������.
                ����� ����� ����������� ��������� �������� �� ��������� � �������,
                � ���������� �������� �������-�������� ���������*/
                // vec_rhs < vec_this
                if (rhs.Size() < size_) {
                    // copy from tranciever to reciever
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    // compute difference size between two vector               
                    size_t dif_size = size_ - rhs.size_;
                    // destroy old elements in domestic vector
                    std::destroy_n(data_.GetAddress() + rhs.size_, dif_size);
                    // assign new size
                    size_ = rhs.size_;
                }
                /*������ �������-��������� ������ ��� ����� ������� �������-��������.
                ����� ����� ��������� ������������ ��������� �������� �������� 
                ��������������� ��������� ���������, � ���������� ����������� � ��������� �������*/
                // vec_rhs >= vec_this
                else if (rhs.Size() >= size_) {
                    // compute difference size between two vector               
                    size_t dif_size = rhs.size_ - size_;
                    if (dif_size == 0) {
                        // copy from tranciever to reciever
                        std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    }
                    if (dif_size > 0) {
                        // copy from tranciever to reciever
                        std::copy_n(rhs.data_.GetAddress(), size_, data_.GetAddress());
                        // copy retained elements
                        std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, dif_size, data_.GetAddress() + size_);
                        // assign new size
                        size_ = rhs.size_;
                    }
                }
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        Swap(rhs);
  
        return *this;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
               
        RawMemory<T> new_data(new_capacity);
        // constexpr �������� if ����� �������� �� ����� ����������
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        // ��������� �������� � data_
        std::destroy_n(data_.GetAddress(), size_);
        // ����������� �� ������ ����� ������, ��������� � �� �����
        data_.Swap(new_data);
        // ��� ������ �� ������ ������ ������ ����� ���������� � ����
    }

    void Swap(Vector& other) noexcept {
        other.data_.Swap(data_);
        std::swap(size_, other.size_);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
        }
        else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            size_ = new_size;
        }
    }

    void PushBack(const T& value) {
       if (size_ == Capacity()) {
           RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
           std::uninitialized_copy_n(&value, 1, new_data.GetAddress() + size_);
           if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
               std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
           }
           else {
               std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
           }
           std::destroy_n(data_.GetAddress(), size_);
           data_.Swap(new_data);
       }
       else {
           new (data_ + size_) T(value);
       }
       ++size_;
    }

    void PushBack(T&& value) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            std::uninitialized_move_n(&value, 1, new_data.GetAddress() + size_);
           if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            new (data_ + size_) T(std::move(value));
        }
        ++size_;
    }

    void PopBack() noexcept {
        if (size_ > 0) {
            std::destroy_at(data_.GetAddress() + size_ - 1);
            --size_;
        }
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        T* result = nullptr;
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            result = new(new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            result = new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *result;
    }

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator begin() const noexcept {
        return const_iterator(data_.GetAddress());
    }
    const_iterator end() const noexcept {
        return const_iterator(data_.GetAddress() + size_);
    }
    const_iterator cbegin() const noexcept {
        return begin();
    }
    const_iterator cend() const noexcept {
        return end();
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        size_t shift = pos - begin();
        iterator result = nullptr;
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            result = new (new_data + shift) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), shift, new_data.GetAddress());
                std::uninitialized_move_n(data_.GetAddress() + shift, size_ - shift, new_data.GetAddress() + shift + 1);
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), shift, new_data.GetAddress());
                std::uninitialized_copy_n(data_.GetAddress() + shift, size_ - shift, new_data.GetAddress() + shift + 1);
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        }
        else {
            if (size_ != 0) {
                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    new (data_ + size_) T(std::move(*(data_.GetAddress() + size_ - 1)));
                    std::move_backward(begin() + shift,
                        data_.GetAddress() + size_,
                        data_.GetAddress() + size_ + 1);
                }
                else {
                    new (data_ + size_) T(std::move(*(data_.GetAddress() + size_ - 1)));
                    std::move_backward(begin() + shift,
                        data_.GetAddress() + size_,
                        data_.GetAddress() + size_ + 1);
                }
                std::destroy_at(begin() + shift);
            }
            result = new (data_ + shift) T(std::forward<Args>(args)...);
        }
        ++size_;
        return result;
    }
    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t shift = pos - begin();
        std::move(begin() + shift + 1, end(), begin() + shift);
        PopBack();
        return begin() + shift;
    }
    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }
    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }
    
    ~Vector() {
       std::destroy_n(data_.GetAddress(), size_);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
    
    // ������ ����� ������� elem � ����� ������ �� ������ buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }

    // �������� ���������� ������� �� ������ buf
   static void Destroy(T* buf) noexcept {
       buf->~T();
   }


};