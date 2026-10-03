/*******************************************************************************
 * File: string.hpp
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

#include "../../tefkernel/patchlib/struct/string.h"
#include "../type.hpp"
#include <string>
#include <memory>

namespace TEFKernel::PatchLib::Struct {
    /**
     * @brief 字符串包装类
     *
     * 提供对C API中字符串操作的C++封装
     */
    class String {
    public:
        /**
         * @brief 默认构造函数
         */
        String() : handle_(PATCH_NULL), ownsHandle_(false) {
        }

        /**
         * @brief 从已有句柄构造
         */
        explicit String(patch_handle_t handle, const bool takeOwnership = false)
            : handle_(handle), ownsHandle_(takeOwnership) {
        }

        /**
         * @brief 从C字符串创建
         */
        explicit String(const char *str)
            : handle_(PATCH_NULL), ownsHandle_(true) {
            handle_ = patchlib_string_create(str);
            if (handle_ == PATCH_NULL) {
                throw std::runtime_error("Failed to create string");
            }
        }

        /**
         * @brief 从std::string创建
         */
        explicit String(const std::string &str)
            : String(str.c_str()) {
        }

        /**
         * @brief 析构函数
         */
        ~String() {
            if (ownsHandle_ && handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                patchlib_free(handle_);
#endif
            }
        }

        // 拷贝构造
        String(const String &other) : handle_(other.handle_), ownsHandle_(false) {
            if (other.ownsHandle_ && other.handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                handle_ = patchlib_handle_copy(other.handle_);
                ownsHandle_ = true;
#endif
            }
        }

        // 拷贝赋值
        String &operator=(const String &other) {
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
        String(String &&other) noexcept
            : handle_(other.handle_), ownsHandle_(other.ownsHandle_) {
            other.handle_ = PATCH_NULL;
            other.ownsHandle_ = false;
        }

        // 移动赋值
        String &operator=(String &&other) noexcept {
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
         * @brief 赋值运算符（从C字符串）
         */
        String &operator=(const char *str) {
            Reset();
            handle_ = patchlib_string_create(str);
            ownsHandle_ = true;
            if (handle_ == PATCH_NULL) {
                throw std::runtime_error("Failed to create string");
            }
            return *this;
        }

        /**
         * @brief 赋值运算符（从std::string）
         */
        String &operator=(const std::string &str) {
            return *this = str.c_str();
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

        // ==================== 字符串操作 ====================

        /**
         * @brief 转换为C字符串
         * @warning 返回的指针需要调用者使用free释放
         */
        [[nodiscard]] std::unique_ptr<char, decltype(&free)> CStr() const {
            char *str = patchlib_string_cstr(handle_);
            // 使用大括号初始化列表，避免重复返回类型
            return {str, free};
        }

        /**
         * @brief 转换为std::string
         */
        [[nodiscard]] std::string ToString() const {
            const auto cstrPtr = CStr();
            return cstrPtr ? std::string(cstrPtr.get()) : std::string();
        }

        /**
         * @brief 获取字符串长度
         */
        [[nodiscard]] size_t Length() const {
            return patchlib_string_length(handle_);
        }

        /**
         * @brief 判断是否为空
         */
        [[nodiscard]] bool Empty() const {
            return patchlib_string_empty(handle_);
        }

        /**
         * @brief 转换为bool（非空返回true）
         */
        explicit operator bool() const {
            return IsValid() && !Empty();
        }

        // ==================== 比较运算符 ====================

        bool operator==(const String &other) const {
            return ToString() == other.ToString();
        }

        bool operator==(const char *str) const {
            return ToString() == str;
        }

        bool operator==(const std::string &str) const {
            return ToString() == str;
        }

        bool operator!=(const String &other) const {
            return !(*this == other);
        }

        bool operator!=(const char *str) const {
            return !(*this == str);
        }

        bool operator!=(const std::string &str) const {
            return !(*this == str);
        }

        bool operator<(const String &other) const {
            return ToString() < other.ToString();
        }

        bool operator>(const String &other) const {
            return ToString() > other.ToString();
        }

        bool operator<=(const String &other) const {
            return ToString() <= other.ToString();
        }

        bool operator>=(const String &other) const {
            return ToString() >= other.ToString();
        }

        // ==================== 友元函数 ====================

        friend bool operator==(const char *lhs, const String &rhs) {
            return rhs == lhs;
        }

        friend bool operator==(const std::string &lhs, const String &rhs) {
            return rhs == lhs;
        }

        friend bool operator!=(const char *lhs, const String &rhs) {
            return rhs != lhs;
        }

        friend bool operator!=(const std::string &lhs, const String &rhs) {
            return rhs != lhs;
        }

    private:
        patch_handle_t handle_;
        bool ownsHandle_;
    };
}
