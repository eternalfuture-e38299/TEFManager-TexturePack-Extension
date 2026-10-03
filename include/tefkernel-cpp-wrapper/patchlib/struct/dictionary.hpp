/*******************************************************************************
 * File: dictionary.hpp
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

#include "../../tefkernel/patchlib/struct/dictionary.h"
#include "../type.hpp"
#include <unordered_map>
#include <string>
#include <type_traits>

namespace TEFKernel::PatchLib::Struct {
    /**
     * @brief 字典包装类
     *
     * 提供对C API中字典操作的C++封装
     *
     * @tparam Key 键类型
     * @tparam Value 值类型
     */
    template<typename Key, typename Value>
    class Dictionary {
    public:
        /**
         * @brief 默认构造函数
         */
        Dictionary() : handle_(PATCH_NULL), ownsHandle_(false) {
        }

        /**
         * @brief 从已有句柄构造
         */
        explicit Dictionary(patch_handle_t handle, const bool takeOwnership = false)
            : handle_(handle), ownsHandle_(takeOwnership) {
        }

        /**
         * @brief 创建字典
         * @param capacity 初始容量
         * @param keyType 键类型句柄（可选）
         * @param valueType 值类型句柄（可选）
         */
        explicit Dictionary(const size_t capacity, patch_handle_t keyType = PATCH_NULL,
                            patch_handle_t valueType = PATCH_NULL)
            : handle_(PATCH_NULL), ownsHandle_(true) {
            if (keyType == PATCH_NULL) {
                keyType = Type::GetBasicType(getKeyPatchType()).GetHandle();
            }
            if (valueType == PATCH_NULL) {
                valueType = Type::GetBasicType(getValuePatchType()).GetHandle();
            }
            handle_ = patchlib_dictionary_create(keyType, valueType, capacity);
            if (handle_ == PATCH_NULL) {
                throw std::runtime_error("Failed to create dictionary");
            }
        }

        /**
         * @brief 从std::unordered_map创建
         */
        explicit Dictionary(const std::unordered_map<Key, Value> &map, size_t capacity = 0,
                            patch_handle_t keyType = PATCH_NULL, patch_handle_t valueType = PATCH_NULL)
            : Dictionary(capacity > 0 ? capacity : map.size(), keyType, valueType) {
            for (const auto &pair: map) {
                add(pair.first, pair.second);
            }
        }

        /**
         * @brief 析构函数
         */
        ~Dictionary() {
            if (ownsHandle_ && handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                patchlib_free(handle_);
#endif
            }
        }

        // 拷贝构造
        Dictionary(const Dictionary &other) : handle_(other.handle_), ownsHandle_(false) {
            if (other.ownsHandle_ && other.handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                handle_ = patchlib_handle_copy(other.handle_);
                ownsHandle_ = true;
#endif
            }
        }

        // 拷贝赋值
        Dictionary &operator=(const Dictionary &other) {
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
        Dictionary(Dictionary &&other) noexcept
            : handle_(other.handle_), ownsHandle_(other.ownsHandle_) {
            other.handle_ = PATCH_NULL;
            other.ownsHandle_ = false;
        }

        // 移动赋值
        Dictionary &operator=(Dictionary &&other) noexcept {
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
        patch_handle_t release() {
            patch_handle_t h = handle_;
            handle_ = PATCH_NULL;
            ownsHandle_ = false;
            return h;
        }

        // ==================== 元素操作 ====================

        /**
         * @brief 添加键值对
         * @throws std::runtime_error 如果键已存在或添加失败
         */
        void Add(const Key &key, const Value &value) {
            if (!patchlib_dictionary_add(handle_,
                                         const_cast<void *>(static_cast<const void *>(&key)),
                                         const_cast<void *>(static_cast<const void *>(&value)))) {
                throw std::runtime_error("Failed to add key-value pair to dictionary");
            }
        }

        /**
         * @brief 获取值
         * @throws std::out_of_range 如果键不存在
         */
        Value Get(const Key &key) const {
            Value value;
            if (!patchlib_dictionary_get_value(handle_,
                                               const_cast<void *>(static_cast<const void *>(&key)),
                                               &value)) {
                throw std::out_of_range("Key not found in dictionary");
            }
            return value;
        }

        /**
         * @brief 尝试获取值
         * @return 是否找到
         */
        bool TryGet(const Key &key, Value &outValue) const {
            return patchlib_dictionary_get_value(handle_,
                                                 const_cast<void *>(static_cast<const void *>(&key)),
                                                 &outValue);
        }

        /**
         * @brief 设置值（如果键存在则修改，否则添加）
         */
        void Set(const Key &key, const Value &value) {
            if (!patchlib_dictionary_set_value(handle_,
                                               const_cast<void *>(static_cast<const void *>(&key)),
                                               const_cast<void *>(static_cast<const void *>(&value)))) {
                throw std::runtime_error("Failed to set value in dictionary");
            }
        }

        /**
         * @brief 移除键值对
         * @return 是否成功移除
         */
        bool Remove(const Key &key) {
            return patchlib_dictionary_remove(handle_,
                                              const_cast<void *>(static_cast<const void *>(&key)));
        }

        /**
         * @brief 清空字典
         */
        void Clear() const {
            if (!patchlib_dictionary_clear(handle_)) {
                throw std::runtime_error("Failed to clear dictionary");
            }
        }

        /**
         * @brief 索引操作符（获取值）
         */
        Value operator[](const Key &key) const {
            return get(key);
        }

        /**
         * @brief 索引操作符（设置值）
         */
        Value &operator[](const Key &key) {
            // 注意：这个实现比较简化，实际应该返回引用
            // 但由于C API限制，我们使用临时值
            static Value temp;
            try {
                temp = get(key);
            } catch (const std::out_of_range &) {
                // 键不存在，添加默认值
                Value defaultVal{};
                add(key, defaultVal);
                temp = defaultVal;
            }
            return temp;
        }

        // ==================== 容量 ====================

        /**
         * @brief 获取长度
         */
        [[nodiscard]] size_t Length() const {
            return patchlib_dictionary_length(handle_);
        }

        /**
         * @brief 判断是否为空
         */
        [[nodiscard]] bool Empty() const {
            return Length() == 0;
        }

        /**
         * @brief 获取大小（同length）
         */
        [[nodiscard]] size_t size() const {
            return Length();
        }

        // ==================== 转换 ====================

        /**
         * @brief 转换为std::unordered_map
         * @note 需要遍历所有键值对，效率较低
         */
        /*
        std::unordered_map<Key, Value> toStdMap() const {
            std::unordered_map<Key, Value> result;
            // 注意：C API没有提供遍历字典的方法
            // 这里需要根据实际API实现遍历
            // 暂时返回空map
            return result;
        }
        */

        // ==================== 运算符 ====================

        explicit operator bool() const {
            return IsValid();
        }

        bool operator==(const Dictionary &other) const {
            // 比较字典内容
            if (Length() != other.Length()) return false;
            // 需要遍历比较所有键值对
            // 这里简化处理，只比较句柄
            return handle_ == other.handle_;
        }

        bool operator!=(const Dictionary &other) const {
            return !(*this == other);
        }

    private:
        static patch_type_t getKeyPatchType() {
            if constexpr (std::is_same_v<Key, void>) return PATCH_VOID;
            else if constexpr (std::is_same_v<Key, int8_t>) return PATCH_INT8;
            else if constexpr (std::is_same_v<Key, int16_t>) return PATCH_INT16;
            else if constexpr (std::is_same_v<Key, int32_t>) return PATCH_INT32;
            else if constexpr (std::is_same_v<Key, int64_t>) return PATCH_INT64;
            else if constexpr (std::is_same_v<Key, uint8_t>) return PATCH_UINT8;
            else if constexpr (std::is_same_v<Key, uint16_t>) return PATCH_UINT16;
            else if constexpr (std::is_same_v<Key, uint32_t>) return PATCH_UINT32;
            else if constexpr (std::is_same_v<Key, uint64_t>) return PATCH_UINT64;
            else if constexpr (std::is_same_v<Key, bool>) return PATCH_BOOL;
            else if constexpr (std::is_same_v<Key, float>) return PATCH_FLOAT;
            else if constexpr (std::is_same_v<Key, double>) return PATCH_DOUBLE;
            else if constexpr (std::is_same_v<Key, char>) return PATCH_CHAR;
            else return PATCH_OBJECT;
        }

        static patch_type_t getValuePatchType() {
            if constexpr (std::is_same_v<Value, void>) return PATCH_VOID;
            else if constexpr (std::is_same_v<Value, int8_t>) return PATCH_INT8;
            else if constexpr (std::is_same_v<Value, int16_t>) return PATCH_INT16;
            else if constexpr (std::is_same_v<Value, int32_t>) return PATCH_INT32;
            else if constexpr (std::is_same_v<Value, int64_t>) return PATCH_INT64;
            else if constexpr (std::is_same_v<Value, uint8_t>) return PATCH_UINT8;
            else if constexpr (std::is_same_v<Value, uint16_t>) return PATCH_UINT16;
            else if constexpr (std::is_same_v<Value, uint32_t>) return PATCH_UINT32;
            else if constexpr (std::is_same_v<Value, uint64_t>) return PATCH_UINT64;
            else if constexpr (std::is_same_v<Value, bool>) return PATCH_BOOL;
            else if constexpr (std::is_same_v<Value, float>) return PATCH_FLOAT;
            else if constexpr (std::is_same_v<Value, double>) return PATCH_DOUBLE;
            else if constexpr (std::is_same_v<Value, char>) return PATCH_CHAR;
            else return PATCH_OBJECT;
        }

        patch_handle_t handle_;
        bool ownsHandle_;
    };

    // ==================== 便利类型别名 ====================

    template<typename Value>
    using StringDictionary = Dictionary<std::string, Value>;

    template<typename Value>
    using Int32Dictionary = Dictionary<int32_t, Value>;

    template<typename Key, typename Value>
    using DictionaryMap = Dictionary<Key, Value>;
}
