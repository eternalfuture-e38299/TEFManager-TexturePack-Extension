/*******************************************************************************
 * File: vector.hpp
 * Project: tefkernel_cpp_wrapper
 * Created: 2026/6/28
 * Author: eternalfuture-e38299
 * Github: https://github.com/eternalfuture-e38299
 *
 * MIT License
 *
 * Copyright (c) 2026 eternalfuture-e38299
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/

#pragma once

#include "../tefkernel/tefstd/vector.h"
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <stdexcept>
#include <iterator>
#include <cstring>
#include <initializer_list>
#include <algorithm>
#include <type_traits>

namespace TEFKernel::Std {
    /**
     * @brief Vector异常类
     */
    class VectorException : public std::runtime_error {
    public:
        explicit VectorException(const std::string &message)
            : std::runtime_error(message) {
        }
    };

    /**
     * @brief 动态数组包装类（类似std::vector）
     *
     * 提供类型安全的C++接口，包装C动态数组实现。
     * 支持自动扩容、随机访问、迭代器等STL兼容特性。
     *
     * @tparam T 元素类型
     */
    template<typename T>
    class Vector {
    public:
        // STL兼容的类型定义
        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;

        /**
         * @brief 迭代器类（支持STL风格遍历）
         */
        class Iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = ptrdiff_t;
            using pointer = T *;
            using reference = T &;

            Iterator() : vec_(nullptr), index_(0) {
            }

            Iterator(Vector *vec, const size_t index) : vec_(vec), index_(index) {
            }

            Iterator &operator++() {
                ++index_;
                return *this;
            }

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            Iterator &operator--() {
                --index_;
                return *this;
            }

            Iterator operator--(int) {
                Iterator tmp = *this;
                --(*this);
                return tmp;
            }

            Iterator &operator+=(const difference_type n) {
                index_ += n;
                return *this;
            }

            Iterator &operator-=(const difference_type n) {
                index_ -= n;
                return *this;
            }

            Iterator operator+(const difference_type n) const {
                return Iterator(vec_, index_ + n);
            }

            Iterator operator-(const difference_type n) const {
                return Iterator(vec_, index_ - n);
            }

            difference_type operator-(const Iterator &other) const {
                return static_cast<difference_type>(index_) - static_cast<difference_type>(other.index_);
            }

            bool operator==(const Iterator &other) const {
                return vec_ == other.vec_ && index_ == other.index_;
            }

            bool operator!=(const Iterator &other) const {
                return !(*this == other);
            }

            bool operator<(const Iterator &other) const {
                return index_ < other.index_;
            }

            bool operator>(const Iterator &other) const {
                return index_ > other.index_;
            }

            bool operator<=(const Iterator &other) const {
                return index_ <= other.index_;
            }

            bool operator>=(const Iterator &other) const {
                return index_ >= other.index_;
            }

            reference operator*() const {
                if (!vec_ || index_ >= vec_->size()) {
                    throw VectorException("Iterator out of bounds");
                }
                return (*vec_)[index_];
            }

            pointer operator->() const {
                return &(**this);
            }

            reference operator[](difference_type n) const {
                return *(*this + n);
            }

            [[nodiscard]] size_t getIndex() const { return index_; }

        private:
            Vector *vec_;
            size_t index_;
        };

        /**
         * @brief Const迭代器类
         */
        class ConstIterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = ptrdiff_t;
            using pointer = const T *;
            using reference = const T &;

            ConstIterator() : vec_(nullptr), index_(0) {
            }

            ConstIterator(const Vector *vec, const size_t index) : vec_(vec), index_(index) {
            }

            ConstIterator &operator++() {
                ++index_;
                return *this;
            }

            ConstIterator operator++(int) {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            ConstIterator &operator--() {
                --index_;
                return *this;
            }

            ConstIterator operator--(int) {
                ConstIterator tmp = *this;
                --(*this);
                return tmp;
            }

            ConstIterator &operator+=(const difference_type n) {
                index_ += n;
                return *this;
            }

            ConstIterator &operator-=(const difference_type n) {
                index_ -= n;
                return *this;
            }

            ConstIterator operator+(const difference_type n) const {
                return ConstIterator(vec_, index_ + n);
            }

            ConstIterator operator-(const difference_type n) const {
                return ConstIterator(vec_, index_ - n);
            }

            difference_type operator-(const ConstIterator &other) const {
                return static_cast<difference_type>(index_) - static_cast<difference_type>(other.index_);
            }

            bool operator==(const ConstIterator &other) const {
                return vec_ == other.vec_ && index_ == other.index_;
            }

            bool operator!=(const ConstIterator &other) const {
                return !(*this == other);
            }

            bool operator<(const ConstIterator &other) const {
                return index_ < other.index_;
            }

            bool operator>(const ConstIterator &other) const {
                return index_ > other.index_;
            }

            bool operator<=(const ConstIterator &other) const {
                return index_ <= other.index_;
            }

            bool operator>=(const ConstIterator &other) const {
                return index_ >= other.index_;
            }

            const_reference operator*() const {
                if (!vec_ || index_ >= vec_->size()) {
                    throw VectorException("Iterator out of bounds");
                }
                return (*vec_)[index_];
            }

            const_pointer operator->() const {
                return &(**this);
            }

            const_reference operator[](difference_type n) const {
                return *(*this + n);
            }

        private:
            const Vector *vec_;
            size_t index_;
        };

        using iterator = Iterator;
        using const_iterator = ConstIterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /**
         * @brief 默认构造函数
         */
        Vector() {
            if (!tefstd_vector_init(&vec_, sizeof(T))) {
                throw VectorException("Failed to initialize vector");
            }
            ownsVec_ = true;
        }

        /**
         * @brief 构造函数（带初始容量）
         * @param initial_capacity 初始容量
         */
        explicit Vector(const size_t initial_capacity) : Vector() {
            reserve(initial_capacity);
        }

        /**
         * @brief 构造函数（填充n个元素）
         * @param n 元素个数
         * @param value 初始值
         */
        Vector(const size_t n, const T &value) : Vector() {
            reserve(n);
            for (size_t i = 0; i < n; ++i) {
                push_back(value);
            }
        }

        /**
         * @brief 从初始化列表构造
         */
        Vector(std::initializer_list<T> init) : Vector() {
            reserve(init.size());
            for (const auto &elem: init) {
                push_back(elem);
            }
        }

        /**
         * @brief 从std::vector构造
         */
        explicit Vector(const std::vector<T> &std_vec) : Vector() {
            reserve(std_vec.size());
            for (const auto &elem: std_vec) {
                push_back(elem);
            }
        }

        /**
         * @brief 从C vector构造（使用C API的init_from_array）
         */
        Vector(const T *array, const size_t length) : Vector() {
            if (!tefstd_vector_init_from_array(&vec_, sizeof(T), const_cast<T *>(array), length)) {
                throw VectorException("Failed to initialize vector from array");
            }
            ownsVec_ = true;
        }

        /**
         * @brief 析构函数
         */
        ~Vector() {
            if (ownsVec_) {
                tefstd_vector_destroy(&vec_);
            }
        }

        // 拷贝构造
        Vector(const Vector &other) : Vector() {
            reserve(other.size());
            for (const auto &elem: other) {
                push_back(elem);
            }
        }

        // 拷贝赋值
        Vector &operator=(const Vector &other) {
            if (this != &other) {
                clear();
                reserve(other.size());
                for (const auto &elem: other) {
                    push_back(elem);
                }
            }
            return *this;
        }

        // 移动构造
        Vector(Vector &&other) noexcept
            : vec_(other.vec_), ownsVec_(other.ownsVec_) {
            other.ownsVec_ = false;
            std::memset(&other.vec_, 0, sizeof(other.vec_));
        }

        // 移动赋值
        Vector &operator=(Vector &&other) noexcept {
            if (this != &other) {
                if (ownsVec_) {
                    tefstd_vector_destroy(&vec_);
                }
                vec_ = other.vec_;
                ownsVec_ = other.ownsVec_;
                other.ownsVec_ = false;
                std::memset(&other.vec_, 0, sizeof(other.vec_));
            }
            return *this;
        }

        /**
         * @brief 赋值运算符（从初始化列表）
         */
        Vector &operator=(std::initializer_list<T> init) {
            clear();
            reserve(init.size());
            for (const auto &elem: init) {
                push_back(elem);
            }
            return *this;
        }

        // ============ 元素访问 ============

        /**
         * @brief 访问指定索引的元素（带边界检查）
         * @throws VectorException 如果索引越界
         */
        reference at(const size_t index) {
            T *elem = static_cast<T *>(tefstd_vector_at(&vec_, index));
            if (!elem) {
                throw VectorException("Index out of bounds");
            }
            return *elem;
        }

        const_reference at(const size_t index) const {
            const T *elem = static_cast<const T *>(tefstd_vector_at(&vec_, index));
            if (!elem) {
                throw VectorException("Index out of bounds");
            }
            return *elem;
        }

        /**
         * @brief 访问指定索引的元素（无边界检查）
         */
        reference operator[](const size_t index) {
            return *static_cast<T *>(tefstd_vector_at(&vec_, index));
        }

        const_reference operator[](const size_t index) const {
            return *static_cast<const T *>(tefstd_vector_at(&vec_, index));
        }

        /**
         * @brief 访问第一个元素
         */
        reference front() {
            if (empty()) {
                throw VectorException("Vector is empty");
            }
            return (*this)[0];
        }

        const_reference front() const {
            if (empty()) {
                throw VectorException("Vector is empty");
            }
            return (*this)[0];
        }

        /**
         * @brief 访问最后一个元素
         */
        reference back() {
            if (empty()) {
                throw VectorException("Vector is empty");
            }
            return (*this)[size() - 1];
        }

        const_reference back() const {
            if (empty()) {
                throw VectorException("Vector is empty");
            }
            return (*this)[size() - 1];
        }

        /**
         * @brief 获取底层数据指针
         */
        pointer data() {
            return static_cast<T *>(vec_.data);
        }

        const_pointer data() const {
            return static_cast<const T *>(vec_.data);
        }

        // ============ 容量 ============

        /**
         * @brief 获取元素个数
         */
        [[nodiscard]] size_t size() const {
            return tefstd_vector_size(&vec_);
        }

        /**
         * @brief 获取容量
         */
        [[nodiscard]] size_t capacity() const {
            return tefstd_vector_capacity(&vec_);
        }

        /**
         * @brief 是否为空
         */
        [[nodiscard]] bool empty() const {
            return size() == 0;
        }

        /**
         * @brief 预分配容量
         */
        void reserve(const size_t new_cap) {
            if (!tefstd_vector_reserve(&vec_, new_cap)) {
                throw VectorException("Failed to reserve memory");
            }
        }

        /**
         * @brief 收缩到合适大小
         */
        void shrink_to_fit() {
            // C API没有直接支持shrink_to_fit，我们通过重新分配实现
            if (size() < capacity()) {
                Vector<T> temp;
                temp.reserve(size());
                for (const auto &elem: *this) {
                    temp.push_back(elem);
                }
                *this = std::move(temp);
            }
        }

        // ============ 修改器 ============

        /**
         * @brief 清空所有元素
         */
        void clear() {
            tefstd_vector_clear(&vec_);
        }

        /**
         * @brief 在末尾添加元素
         */
        void push_back(const T &value) {
            if (!tefstd_vector_push_back(&vec_, &value)) {
                throw VectorException("Failed to push back element");
            }
        }

        /**
         * @brief 在末尾添加元素（移动语义）
         */
        void push_back(T &&value) {
            push_back(value); // 简单实现，C API不支持移动
        }

        /**
         * @brief 在末尾构造元素
         */
        template<typename... Args>
        reference emplace_back(Args &&... args) {
            T value(std::forward<Args>(args)...);
            push_back(value);
            return back();
        }

        /**
         * @brief 删除最后一个元素
         */
        void pop_back() {
            if (empty()) {
                throw VectorException("Vector is empty");
            }
            T temp;
            if (!tefstd_vector_pop_back(&vec_, &temp)) {
                throw VectorException("Failed to pop back element");
            }
        }

        /**
         * @brief 删除指定位置的元素
         */
        iterator erase(iterator pos) {
            size_t index = pos.getIndex();
            if (index >= size()) {
                throw VectorException("Iterator out of bounds");
            }

            T temp;
            if (!tefstd_vector_erase(&vec_, index, &temp)) {
                throw VectorException("Failed to erase element");
            }

            return iterator(this, index);
        }

        /**
         * @brief 删除范围内的元素
         */
        iterator erase(iterator first, iterator last) {
            size_t first_idx = first.getIndex();
            const size_t last_idx = last.getIndex();

            if (first_idx >= size() || last_idx > size() || first_idx > last_idx) {
                throw VectorException("Invalid range");
            }

            // 逐个删除
            for (size_t i = first_idx; i < last_idx; ++i) {
                tefstd_vector_erase(&vec_, first_idx, nullptr);
            }

            return iterator(this, first_idx);
        }

        /**
         * @brief 删除所有匹配值的元素
         */
        bool remove(const T &value) {
            return tefstd_vector_remove_value(&vec_, &value);
        }

        /**
         * @brief 插入元素到指定位置
         */
        iterator insert(iterator pos, const T &value) {
            size_t index = pos.getIndex();
            if (index > size()) {
                throw VectorException("Invalid insert position");
            }

            // 在末尾添加，然后移动到指定位置
            push_back(value);
            if (index < size() - 1) {
                // 将新元素移到指定位置
                for (size_t i = size() - 1; i > index; --i) {
                    std::swap((*this)[i], (*this)[i - 1]);
                }
            }

            return iterator(this, index);
        }

        /**
         * @brief 插入多个相同元素
         */
        iterator insert(iterator pos, const size_t count, const T &value) {
            size_t index = pos.getIndex();
            if (index > size()) {
                throw VectorException("Invalid insert position");
            }

            for (size_t i = 0; i < count; ++i) {
                insert(iterator(this, index + i), value);
            }

            return iterator(this, index);
        }

        /**
         * @brief 交换两个Vector的内容
         */
        void swap(Vector &other) noexcept {
            std::swap(vec_, other.vec_);
            std::swap(ownsVec_, other.ownsVec_);
        }

        // ============ 迭代器 ============

        iterator begin() {
            return iterator(this, 0);
        }

        iterator end() {
            return iterator(this, size());
        }

        const_iterator begin() const {
            return const_iterator(this, 0);
        }

        const_iterator end() const {
            return const_iterator(this, size());
        }

        const_iterator cbegin() const {
            return begin();
        }

        const_iterator cend() const {
            return end();
        }

        reverse_iterator rbegin() {
            return reverse_iterator(end());
        }

        reverse_iterator rend() {
            return reverse_iterator(begin());
        }

        const_reverse_iterator rbegin() const {
            return const_reverse_iterator(end());
        }

        const_reverse_iterator rend() const {
            return const_reverse_iterator(begin());
        }

        const_reverse_iterator crbegin() const {
            return rbegin();
        }

        const_reverse_iterator crend() const {
            return rend();
        }

        // ============ 转换为STL容器 ============

        /**
         * @brief 转换为std::vector
         */
        std::vector<T> toStdVector() const {
            std::vector<T> result;
            result.reserve(size());
            for (const auto &elem: *this) {
                result.push_back(elem);
            }
            return result;
        }

        /**
         * @brief 获取C指针
         */
        tefstd_vector_t *getHandle() {
            return &vec_;
        }

        /**
         * @brief 获取C指针（const版本）
         */
        [[nodiscard]] const tefstd_vector_t *getHandle() const {
            return &vec_;
        }

        // ============ 比较运算符 ============

        bool operator==(const Vector &other) const {
            if (size() != other.size()) return false;
            for (size_t i = 0; i < size(); ++i) {
                if ((*this)[i] != other[i]) return false;
            }
            return true;
        }

        bool operator!=(const Vector &other) const {
            return !(*this == other);
        }

        bool operator<(const Vector &other) const {
            return std::lexicographical_compare(
                begin(), end(), other.begin(), other.end()
            );
        }

        bool operator>(const Vector &other) const {
            return other < *this;
        }

        bool operator<=(const Vector &other) const {
            return !(*this > other);
        }

        bool operator>=(const Vector &other) const {
            return !(*this < other);
        }

    private:
        tefstd_vector_t vec_{};
        bool ownsVec_ = true;

        // 友元迭代器
        friend class Iterator;
        friend class ConstIterator;
    };

    // ============ 便利类型定义 ============

    using IntVector = Vector<int>;
    using DoubleVector = Vector<double>;
    using StringVector = Vector<std::string>;
    using FloatVector = Vector<float>;

    // ============ 便利函数 ============
    /**
     * @brief 创建Vector的智能指针
     */
    template<typename T>
    std::unique_ptr<Vector<T> > makeVector() {
        return std::make_unique<Vector<T> >();
    }

    template<typename T>
    std::unique_ptr<Vector<T> > makeVector(size_t n, const T &value) {
        return std::make_unique<Vector<T> >(n, value);
    }

    template<typename T>
    std::unique_ptr<Vector<T> > makeVector(std::initializer_list<T> init) {
        return std::make_unique<Vector<T> >(init);
    }
}
