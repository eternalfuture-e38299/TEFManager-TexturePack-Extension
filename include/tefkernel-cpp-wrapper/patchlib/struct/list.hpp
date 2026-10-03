/*******************************************************************************
 * File: list.hpp
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

#include "../../tefkernel/patchlib/struct/list.h"
#include "../type.hpp"
#include "array.hpp"
#include <vector>
#include <type_traits>

namespace TEFKernel::PatchLib::Struct {
    /**
     * @brief 列表包装类
     *
     * 提供对C API中列表操作的C++封装
     *
     * @tparam T 元素类型
     */
    template<typename T>
    class List {
    public:
        /**
         * @brief 默认构造函数
         */
        List() : handle_(PATCH_NULL), ownsHandle_(false) {
        }

        /**
         * @brief 从已有句柄构造
         */
        explicit List(patch_handle_t handle, const bool takeOwnership = false)
            : handle_(handle), ownsHandle_(takeOwnership) {
        }

        /**
         * @brief 创建列表
         * @param capacity 初始容量
         * @param type 元素类型句柄（可选）
         */
        explicit List(const size_t capacity, patch_handle_t type = PATCH_NULL)
            : handle_(PATCH_NULL), ownsHandle_(true) {
            if (type == PATCH_NULL) {
                type = Type::GetBasicType(GetPatchType()).GetHandle();
            }
            handle_ = patchlib_list_create(capacity, type);
            if (handle_ == PATCH_NULL) {
                throw std::runtime_error("Failed to create list");
            }
        }

        /**
         * @brief 从std::vector创建
         */
        explicit List(const std::vector<T> &vec, patch_handle_t type = PATCH_NULL)
            : List(vec.size(), type) {
            for (const auto &elem: vec) {
                add(elem);
            }
        }

        /**
         * @brief 从Array复制创建
         */
        explicit List(const Array<T> &array, patch_handle_t type = PATCH_NULL)
            : List(array.length(), type) {
            if (!patchlib_list_copy_from(handle_, array.getHandle())) {
                throw std::runtime_error("Failed to copy from array");
            }
        }

        /**
         * @brief 析构函数
         */
        ~List() {
            if (ownsHandle_ && handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                patchlib_free(handle_);
#endif
            }
        }

        // 拷贝构造
        List(const List &other) : handle_(other.handle_), ownsHandle_(false) {
            if (other.ownsHandle_ && other.handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                handle_ = patchlib_handle_copy(other.handle_);
                ownsHandle_ = true;
#endif
            }
        }

        // 拷贝赋值
        List &operator=(const List &other) {
            if (this != &other) {
                Reset();
                handle_ = other.handle_;
                ownsHandle_ = false;
                if (other.ownsHandle_ && other.handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                    handle_ = patchlib_handle_copy(other.handle_);
                    ownsHandle_ = true;
#endif
                }
            }
            return *this;
        }

        // 移动构造
        List(List &&other) noexcept
            : handle_(other.handle_), ownsHandle_(other.ownsHandle_) {
            other.handle_ = PATCH_NULL;
            other.ownsHandle_ = false;
        }

        // 移动赋值
        List &operator=(List &&other) noexcept {
            if (this != &other) {
                Reset();
                handle_ = other.handle_;
                ownsHandle_ = other.ownsHandle_;
                other.handle_ = PATCH_NULL;
                other.ownsHandle_ = false;
            }
            return *this;
        }

        /**
         * @brief 获取原始句柄
         */
        [[nodiscard]] patch_handle_t GetHandle() const { return handle_; }

        /**
         * @brief 检查是否有效
         */
        [[nodiscard]] bool IsValid() const {
            return patchlib_is_valid(handle_);
        }

        /**
         * @brief 重置句柄
         */
        void Reset(patch_handle_t newHandle = PATCH_NULL, const bool takeOwnership = false) {
            if (ownsHandle_ && handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                patchlib_free(handle_);
#endif
            }
            handle_ = newHandle;
            ownsHandle_ = takeOwnership;
        }

        /**
         * @brief 释放所有权
         */
        patch_handle_t Release() {
            patch_handle_t h = handle_;
            handle_ = PATCH_NULL;
            ownsHandle_ = false;
            return h;
        }

        // ==================== 元素操作 ====================

        /**
         * @brief 添加元素
         */
        void add(const T &value) {
            if (!patchlib_list_add(handle_, const_cast<void *>(static_cast<const void *>(&value)))) {
                throw std::runtime_error("Failed to add element to list");
            }
        }

        /**
         * @brief 移除元素
         */
        bool remove(const T &value) {
            return patchlib_list_remove(handle_, const_cast<void *>(static_cast<const void *>(&value)));
        }

        /**
         * @brief 移除指定索引的元素
         * @throws std::out_of_range 如果索引越界
         */
        void removeAt(const size_t index) const {
            if (!patchlib_list_remove_at(handle_, index)) {
                throw std::out_of_range("List index out of bounds");
            }
        }

        /**
         * @brief 清空列表
         */
        void clear() const {
            if (!patchlib_list_clear(handle_)) {
                throw std::runtime_error("Failed to clear list");
            }
        }

        // ==================== 容量 ====================

        /**
         * @brief 获取长度
         */
        [[nodiscard]] size_t length() const {
            return getArray().length();
        }

        /**
         * @brief 判断是否为空
         */
        [[nodiscard]] bool empty() const {
            return length() == 0;
        }

        /**
         * @brief 获取大小（同length）
         */
        [[nodiscard]] size_t size() const {
            return length();
        }

        // ==================== 转换 ====================

        /**
         * @brief 获取内部数组
         */
        Array<T> getArray() const {
            return Array<T>(patchlib_list_get_array(handle_), false);
        }

        /**
         * @brief 转换为std::vector
         */
        std::vector<T> toStdVector() const {
            Array<T> arr = getArray();
            return arr.toStdVector();
        }

        // ==================== 迭代器支持 ====================

        /**
         * @brief 简单迭代器
         */
        class Iterator {
        public:
            Iterator(List *list, const size_t index) : list_(list), index_(index) {
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

            T &operator*() {
                Array<T> arr = list_->getArray();
                return const_cast<T &>(arr[index_]);
            }

            bool operator==(const Iterator &other) const {
                return list_ == other.list_ && index_ == other.index_;
            }

            bool operator!=(const Iterator &other) const {
                return !(*this == other);
            }

        private:
            List *list_;
            size_t index_;
        };

        class ConstIterator {
        public:
            ConstIterator(const List *list, const size_t index) : list_(list), index_(index) {
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

            const T &operator*() {
                Array<T> arr = list_->getArray();
                return arr[index_];
            }

            bool operator==(const ConstIterator &other) const {
                return list_ == other.list_ && index_ == other.index_;
            }

            bool operator!=(const ConstIterator &other) const {
                return !(*this == other);
            }

        private:
            const List *list_;
            size_t index_;
        };

        Iterator Begin() { return Iterator(this, 0); }
        Iterator End() { return Iterator(this, length()); }
        ConstIterator Begin() const { return ConstIterator(this, 0); }
        ConstIterator End() const { return ConstIterator(this, length()); }
        ConstIterator CBegin() const { return ConstIterator(this, 0); }
        ConstIterator CEnd() const { return ConstIterator(this, length()); }

        // ==================== 运算符 ====================

        explicit operator bool() const {
            return IsValid();
        }

        bool operator==(const List &other) const {
            if (length() != other.length()) return false;
            Array<T> arr1 = getArray();
            Array<T> arr2 = other.getArray();
            for (size_t i = 0; i < length(); ++i) {
                if (arr1[i] != arr2[i]) return false;
            }
            return true;
        }

        bool operator!=(const List &other) const {
            return !(*this == other);
        }

    private:
        static patch_type_t GetPatchType() {
            if constexpr (std::is_same_v<T, void>) return PATCH_VOID;
            else if constexpr (std::is_same_v<T, int8_t>) return PATCH_INT8;
            else if constexpr (std::is_same_v<T, int16_t>) return PATCH_INT16;
            else if constexpr (std::is_same_v<T, int32_t>) return PATCH_INT32;
            else if constexpr (std::is_same_v<T, int64_t>) return PATCH_INT64;
            else if constexpr (std::is_same_v<T, uint8_t>) return PATCH_UINT8;
            else if constexpr (std::is_same_v<T, uint16_t>) return PATCH_UINT16;
            else if constexpr (std::is_same_v<T, uint32_t>) return PATCH_UINT32;
            else if constexpr (std::is_same_v<T, uint64_t>) return PATCH_UINT64;
            else if constexpr (std::is_same_v<T, bool>) return PATCH_BOOL;
            else if constexpr (std::is_same_v<T, float>) return PATCH_FLOAT;
            else if constexpr (std::is_same_v<T, double>) return PATCH_DOUBLE;
            else if constexpr (std::is_same_v<T, char>) return PATCH_CHAR;
            else return PATCH_OBJECT;
        }

        patch_handle_t handle_;
        bool ownsHandle_;
    };

    // ==================== 便利类型别名 ====================

    using Int8List = List<int8_t>;
    using Int16List = List<int16_t>;
    using Int32List = List<int32_t>;
    using Int64List = List<int64_t>;
    using UInt8List = List<uint8_t>;
    using UInt16List = List<uint16_t>;
    using UInt32List = List<uint32_t>;
    using UInt64List = List<uint64_t>;
    using BoolList = List<bool>;
    using FloatList = List<float>;
    using DoubleList = List<double>;
    using CharList = List<char>;
}
