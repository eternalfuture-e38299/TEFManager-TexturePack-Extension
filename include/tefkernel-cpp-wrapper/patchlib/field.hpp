/*******************************************************************************
 * File: field.hpp
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

#include "../tefkernel/patchlib/field.h"
#include "type.hpp"
#include <string>

namespace TEFKernel::PatchLib {
    /**
     * @brief 字段包装类
     */
    class Field {
    public:
        Field() : handle_(PATCH_NULL), ownsHandle_(false) {
        }

        explicit Field(patch_handle_t handle, const bool takeOwnership = false)
            : handle_(handle), ownsHandle_(takeOwnership) {
        }

        ~Field() {
            if (ownsHandle_ && handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                patchlib_free(handle_);
#endif
            }
        }

        // 拷贝构造
        Field(const Field &other) : handle_(other.handle_), ownsHandle_(false) {
            if (other.ownsHandle_ && other.handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                handle_ = patchlib_handle_copy(other.handle_);
                ownsHandle_ = true;
#endif
            }
        }

        // 拷贝赋值
        Field &operator=(const Field &other) {
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
        Field(Field &&other) noexcept
            : handle_(other.handle_), ownsHandle_(other.ownsHandle_) {
            other.handle_ = PATCH_NULL;
            other.ownsHandle_ = false;
        }

        // 移动赋值
        Field &operator=(Field &&other) noexcept {
            if (this != &other) {
                Reset();
                handle_ = other.handle_;
                ownsHandle_ = other.ownsHandle_;
                other.handle_ = PATCH_NULL;
                other.ownsHandle_ = false;
            }
            return *this;
        }

        [[nodiscard]] patch_handle_t GetHandle() const { return handle_; }

        [[nodiscard]] bool IsValid() const {
            return patchlib_is_valid(handle_);
        }

        void Reset(patch_handle_t newHandle = PATCH_NULL, const bool takeOwnership = false) {
            if (ownsHandle_ && handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                patchlib_free(handle_);
#endif
            }
            handle_ = newHandle;
            ownsHandle_ = takeOwnership;
        }

        patch_handle_t Release() {
            patch_handle_t h = handle_;
            handle_ = PATCH_NULL;
            ownsHandle_ = false;
            return h;
        }

        // ==================== 基本信息 ====================

        [[nodiscard]] std::string GetName() const {
            const char *name = patchlib_field_get_name(handle_);
            return name ? std::string(name) : std::string();
        }

        [[nodiscard]] bool IsStatic() const {
            return patchlib_field_is_static(handle_);
        }

        [[nodiscard]] bool IsInstance() const {
            return patchlib_field_is_instance(handle_);
        }

        [[nodiscard]] bool IsConst() const {
            return patchlib_field_is_const(handle_);
        }

        [[nodiscard]] bool IsThreadStatic() const {
            return patchlib_field_is_thread_static(handle_);
        }

        // ==================== 值操作 ====================

        /**
         * @brief 获取字段值（模板版本）
         */
        template<typename T>
        T GetValue(patch_handle_t instance = PATCH_NULL) const {
            T value;
            patchlib_field_get_value(handle_, instance, &value);
            return value;
        }

        /**
         * @brief 获取字段值（void*版本）
         */
        void GetValue(patch_handle_t instance, void *value) const {
            patchlib_field_get_value(handle_, instance, value);
        }

        /**
         * @brief 设置字段值（模板版本）
         */
        template<typename T>
        void SetValue(patch_handle_t instance, const T &value) const {
            patchlib_field_set_value(handle_, instance, const_cast<void *>(static_cast<const void *>(&value)));
        }

        /**
         * @brief 设置字段值（void*版本）
         */
        void SetValue(patch_handle_t instance, void *value) const {
            patchlib_field_set_value(handle_, instance, value);
        }

        // ==================== 类型信息 ====================

        [[nodiscard]] size_t GetSize() const {
            return patchlib_field_get_size(handle_);
        }

        [[nodiscard]] patch_type_t GetType() const {
            return patchlib_field_get_type(handle_);
        }

#if __ANDROID__
        [[nodiscard]] void *GetPointer(void *instance) const {
            return patchlib_field_get_pointer(handle_, instance);
        }
#endif

        // ==================== 运算符 ====================

        explicit operator bool() const {
            return IsValid();
        }

        bool operator==(const Field &other) const {
            return handle_ == other.handle_;
        }

        bool operator!=(const Field &other) const {
            return !(*this == other);
        }

    private:
        patch_handle_t handle_;
        bool ownsHandle_;
    };

    // Type类中需要的方法实现（避免循环依赖）
    inline Field Type::GetField(const std::string &name) const {
        return Field(patchlib_type_get_field(handle_, name.c_str()), false);
    }

    inline Std::Vector<Field> Type::GetFields(const bool includingParent) const {
        tefstd_vector_t vec;
        if (!tefstd_vector_init(&vec, sizeof(patch_handle_t))) {
            return {};
        }

        if (patchlib_type_get_fields(handle_, includingParent, &vec)) {
            Std::Vector<Field> result;
            const auto *data = static_cast<patch_handle_t *>(vec.data);
            for (size_t i = 0; i < vec.size; ++i) {
                result.push_back(Field(data[i], false));
            }
            tefstd_vector_destroy(&vec);
            return result;
        }

        tefstd_vector_destroy(&vec);
        return {};
    }
}