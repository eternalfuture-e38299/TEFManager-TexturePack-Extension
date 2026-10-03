/*******************************************************************************
 * File: array.hpp
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

#include <cstdint>
#include <vector>
#include <type_traits>
#include <stdexcept>

#include "../../tefkernel/patchlib/struct/array.h"
#include "../type.hpp"

namespace TEFKernel::PatchLib::Struct {

    /**
     * @brief 数组包装类
     *
     * 提供对C API中数组操作的C++封装
     *
     * @tparam T 元素类型
     */
    template<typename T>
    class Array {
    public:
        /**
         * @brief 默认构造函数
         */
        Array() : Handle_(PATCH_NULL), OwnsHandle_(false) {
        }

        /**
         * @brief 从已有句柄构造
         * @param handle 数组句柄
         * @param takeOwnership 是否取得所有权
         */
        explicit Array(patch_handle_t handle, const bool takeOwnership = false)
            : Handle_(handle), OwnsHandle_(takeOwnership) {
        }

        /**
         * @brief 创建数组
         * @param size 数组大小
         * @param type 元素类型句柄（可选，如果不提供则自动推断）
         */
        explicit Array(const size_t size, patch_handle_t type = PATCH_NULL)
            : Handle_(PATCH_NULL), OwnsHandle_(true) {
            if (type == PATCH_NULL) {
                // 尝试自动推断类型
                type = Type::GetBasicType(GetPatchType()).GetHandle();
            }
            Handle_ = patchlib_array_create(size, type);
            if (Handle_ == PATCH_NULL) {
                throw std::runtime_error("Failed to create array");
            }
        }

        /**
         * @brief 从C数组创建
         * @param data C数组数据
         * @param size 元素个数
         * @param type 元素类型句柄
         */
        Array(const T *data, const size_t size, patch_handle_t type = PATCH_NULL)
            : Array(size, type) {
            if (!patchlib_array_copy_from_c(Handle_, data, size)) {
                throw std::runtime_error("Failed to copy from C array");
            }
            OwnsHandle_ = true;
        }

        /**
         * @brief 从std::vector创建
         */
        explicit Array(const std::vector<T> &vec, patch_handle_t type = PATCH_NULL)
            : Array(vec.data(), vec.size(), type) {
        }

        /**
         * @brief 析构函数
         */
        ~Array() {
            if (OwnsHandle_ && Handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                patchlib_free(Handle_);
#endif
            }
        }

        // 拷贝构造
        Array(const Array &other) : Handle_(other.Handle_), OwnsHandle_(false) {
            if (other.OwnsHandle_ && other.Handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                Handle_ = patchlib_handle_copy(other.Handle_);
                OwnsHandle_ = true;
#endif
            }
        }

        // 拷贝赋值
        Array &operator=(const Array &other) {
            if (this != &other) {
                Reset();
                Handle_ = other.Handle_;
                OwnsHandle_ = false;
                if (other.OwnsHandle_ && other.Handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                    Handle_ = patchlib_handle_copy(other.Handle_);
                    OwnsHandle_ = true;
#endif
                }
            }
            return *this;
        }

        // 移动构造
        Array(Array &&other) noexcept
            : Handle_(other.Handle_), OwnsHandle_(other.OwnsHandle_) {
            other.Handle_ = PATCH_NULL;
            other.OwnsHandle_ = false;
        }

        // 移动赋值
        Array &operator=(Array &&other) noexcept {
            if (this != &other) {
                Reset();
                Handle_ = other.Handle_;
                OwnsHandle_ = other.OwnsHandle_;
                other.Handle_ = PATCH_NULL;
                other.OwnsHandle_ = false;
            }
            return *this;
        }

        /**
         * @brief 获取原始句柄
         */
        [[nodiscard]] patch_handle_t GetHandle() const { return Handle_; }

        /**
         * @brief 检查是否有效
         */
        [[nodiscard]] bool IsValid() const {
            return patchlib_is_valid(Handle_);
        }

        /**
         * @brief 重置句柄
         */
        void Reset(patch_handle_t newHandle = PATCH_NULL, const bool takeOwnership = false) {
            if (OwnsHandle_ && Handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                patchlib_free(Handle_);
#endif
            }
            Handle_ = newHandle;
            OwnsHandle_ = takeOwnership;
        }

        /**
         * @brief 释放所有权
         */
        patch_handle_t Release() {
            patch_handle_t h = Handle_;
            Handle_ = PATCH_NULL;
            OwnsHandle_ = false;
            return h;
        }

        // ==================== 元素访问 ====================

        /**
         * @brief 获取元素（带边界检查）
         * @throws std::out_of_range 如果索引越界
         */
        T At(const size_t index) const {
            T value;
            if (!patchlib_array_at(Handle_, index, &value)) {
                throw std::out_of_range("Array index out of bounds");
            }
            return value;
        }

        /**
         * @brief 获取元素（无边界检查）
         */
        T operator[](const size_t index) const {
            T value;
            patchlib_array_at(Handle_, index, &value);
            return value;
        }

        /**
         * @brief 设置元素（带边界检查）
         * @throws std::out_of_range 如果索引越界
         */
        void Set(const size_t index, const T &value) {
            if (!patchlib_array_set(Handle_, index, const_cast<void *>(static_cast<const void *>(&value)))) {
                throw std::out_of_range("Array index out of bounds");
            }
        }

        /**
         * @brief 设置元素（无边界检查）
         */
        void SetUnchecked(const size_t index, const T &value) {
            patchlib_array_set(Handle_, index, const_cast<void *>(static_cast<const void *>(&value)));
        }

        // ==================== 容量 ====================

        /**
         * @brief 获取数组长度
         */
        [[nodiscard]] size_t Length() const {
            return patchlib_array_length(Handle_);
        }

        /**
         * @brief 判断是否为空
         */
        [[nodiscard]] bool IsEmpty() const {
            return patchlib_array_empty(Handle_);
        }

        /**
         * @brief 获取元素个数（同Length）
         */
        [[nodiscard]] size_t Size() const {
            return Length();
        }

        // ==================== 填充 ====================

        /**
         * @brief 填充所有元素
         */
        void Fill(const T &value) {
            if (!patchlib_array_fill(Handle_, const_cast<void *>(static_cast<const void *>(&value)))) {
                throw std::runtime_error("Failed to fill array");
            }
        }

        // ==================== 复制操作 ====================

        /**
         * @brief 复制到C数组
         */
        void CopyToC(T *dest, const size_t count) const {
            if (!patchlib_array_copy_to_c(dest, Handle_, count)) {
                throw std::runtime_error("Failed to copy to C array");
            }
        }

        /**
         * @brief 从C数组复制
         */
        void CopyFromC(const T *src, const size_t count) {
            if (!patchlib_array_copy_from_c(Handle_, src, count)) {
                throw std::runtime_error("Failed to copy from C array");
            }
        }

        /**
         * @brief 转换为std::vector
         */
        std::vector<T> ToStdVector() const {
            std::vector<T> result;
            result.resize(Length());
            CopyToC(result.data(), result.size());
            return result;
        }

        // ==================== 迭代器支持 ====================

        /**
         * @brief 简单迭代器类
         */
        class Iterator {
        public:
            Iterator(Array *array, const size_t index) : Array_(array), Index_(index) {
            }

            Iterator &operator++() {
                ++Index_;
                return *this;
            }

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            Iterator &operator--() {
                --Index_;
                return *this;
            }

            Iterator operator--(int) {
                Iterator tmp = *this;
                --(*this);
                return tmp;
            }

            T operator*() const { return (*Array_)[Index_]; }

            bool operator==(const Iterator &other) const {
                return Array_ == other.Array_ && Index_ == other.Index_;
            }

            bool operator!=(const Iterator &other) const {
                return !(*this == other);
            }

        private:
            Array *Array_;
            size_t Index_;
        };

        class ConstIterator {
        public:
            ConstIterator(const Array *array, const size_t index) : Array_(array), Index_(index) {
            }

            ConstIterator &operator++() {
                ++Index_;
                return *this;
            }

            ConstIterator operator++(int) {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            ConstIterator &operator--() {
                --Index_;
                return *this;
            }

            ConstIterator operator--(int) {
                ConstIterator tmp = *this;
                --(*this);
                return tmp;
            }

            T operator*() const { return (*Array_)[Index_]; }

            bool operator==(const ConstIterator &other) const {
                return Array_ == other.Array_ && Index_ == other.Index_;
            }

            bool operator!=(const ConstIterator &other) const {
                return !(*this == other);
            }

        private:
            const Array *Array_;
            size_t Index_;
        };

        Iterator Begin() { return Iterator(this, 0); }
        Iterator End() { return Iterator(this, Length()); }
        ConstIterator Begin() const { return ConstIterator(this, 0); }
        ConstIterator End() const { return ConstIterator(this, Length()); }
        ConstIterator CBegin() const { return ConstIterator(this, 0); }
        ConstIterator CEnd() const { return ConstIterator(this, Length()); }

        // ==================== 运算符 ====================

        explicit operator bool() const {
            return IsValid();
        }

        bool operator==(const Array &other) const {
            if (Length() != other.Length()) return false;
            for (size_t i = 0; i < Length(); ++i) {
                if ((*this)[i] != other[i]) return false;
            }
            return true;
        }

        bool operator!=(const Array &other) const {
            return !(*this == other);
        }

        // ==================== 静态工厂方法 ====================

        /**
         * @brief 创建空数组
         */
        static Array CreateEmpty() {
            return Array(0);
        }

        /**
         * @brief 从初始化列表创建
         */
        static Array FromInitializerList(const std::initializer_list<T> &list) {
            return Array(std::vector<T>(list));
        }

    private:
        /**
         * @brief 获取对应Patch类型
         */
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
            else if constexpr (std::is_same_v<T, void*>) return PATCH_POINTER;
            else return PATCH_OBJECT;
        }

        patch_handle_t Handle_;
        bool OwnsHandle_;
    };

    // ==================== 便利类型别名 ====================

    using Int8Array = Array<int8_t>;
    using Int16Array = Array<int16_t>;
    using Int32Array = Array<int32_t>;
    using Int64Array = Array<int64_t>;
    using UInt8Array = Array<uint8_t>;
    using UInt16Array = Array<uint16_t>;
    using UInt32Array = Array<uint32_t>;
    using UInt64Array = Array<uint64_t>;
    using BoolArray = Array<bool>;
    using FloatArray = Array<float>;
    using DoubleArray = Array<double>;
    using CharArray = Array<char>;
    using WCharArray = Array<wchar_t>;
    using PointerArray = Array<void*>;

    // ==================== 辅助函数 ====================

    /**
     * @brief 创建数组的便捷函数
     */
    template<typename T>
    Array<T> MakeArray(const size_t size) {
        return Array<T>(size);
    }

    /**
     * @brief 从vector创建数组的便捷函数
     */
    template<typename T>
    Array<T> MakeArrayFromVector(const std::vector<T> &vec) {
        return Array<T>(vec);
    }

    /**
     * @brief 从初始化列表创建数组的便捷函数
     */
    template<typename T>
    Array<T> MakeArrayFromList(const std::initializer_list<T> &list) {
        return Array<T>::FromInitializerList(list);
    }

} // namespace TEFKernel::PatchLib::Struct