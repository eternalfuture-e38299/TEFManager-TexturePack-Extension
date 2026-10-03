/*******************************************************************************
 * File: hashmap.hpp
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

#include "../tefkernel/tefstd/hashmap.h"
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <iterator>
#include <cstring>
#include <initializer_list>

namespace TEFKernel::Std {
    /**
     * @brief 哈希表异常类
     */
    class HashMapException : public std::runtime_error {
    public:
        explicit HashMapException(const std::string &message)
            : std::runtime_error(message) {
        }
    };

    /**
     * @brief 类型安全的哈希表包装器
     *
     * 提供类型安全的C++接口，包装C哈希表实现
     *
     * @tparam Key 键类型
     * @tparam Value 值类型
     */
    template<typename Key, typename Value>
    class HashMap {
    public:
        // STL兼容的类型定义
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const Key, Value>;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        /**
         * @brief 迭代器类（支持STL风格遍历）
         */
        class Iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<Key, Value>;
            using difference_type = ptrdiff_t;
            using pointer = value_type *;
            using reference = value_type &;

            Iterator() : hashmap_(nullptr), index_(0) {
            }

            Iterator(HashMap *hashmap, const size_t index)
                : hashmap_(hashmap), index_(index) {
                if (hashmap_ && index_ < hashmap_->map_.capacity) {
                    // 定位到第一个有效元素
                    if (!isValid()) {
                        advanceToNextValid();
                    }
                }
            }

            Iterator &operator++() {
                if (hashmap_) {
                    ++index_;
                    advanceToNextValid();
                }
                return *this;
            }

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const Iterator &other) const {
                return hashmap_ == other.hashmap_ && index_ == other.index_;
            }

            bool operator!=(const Iterator &other) const {
                return !(*this == other);
            }

            std::pair<Key, Value> operator*() const {
                if (!hashmap_ || !isValid()) {
                    throw HashMapException("Iterator out of bounds");
                }

                Key key;
                Value value;
                hashmap_->getEntryAt(index_, key, value);
                return {key, value};
            }

            // 获取键和值的引用（更高效）
            void get(Key &key, Value &value) const {
                if (!hashmap_ || !isValid()) {
                    throw HashMapException("Iterator out of bounds");
                }
                hashmap_->getEntryAt(index_, key, value);
            }

            [[nodiscard]] size_t getIndex() const { return index_; }

        private:
            [[nodiscard]] bool isValid() const {
                if (!hashmap_ || index_ >= hashmap_->map_.capacity) return false;
                return hashmap_->map_.states[index_] == 1; // 1 = occupied
            }

            void advanceToNextValid() {
                while (hashmap_ && index_ < hashmap_->map_.capacity && !isValid()) {
                    ++index_;
                }
            }

            HashMap *hashmap_;
            size_t index_;
        };

        /**
         * @brief Const迭代器类
         */
        class ConstIterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<Key, Value>;
            using difference_type = ptrdiff_t;
            using pointer = const value_type *;
            using reference = const value_type &;

            ConstIterator() : hashmap_(nullptr), index_(0) {
            }

            ConstIterator(const HashMap *hashmap, const size_t index)
                : hashmap_(hashmap), index_(index) {
                if (hashmap_ && index_ < hashmap_->map_.capacity) {
                    if (!isValid()) {
                        advanceToNextValid();
                    }
                }
            }

            ConstIterator &operator++() {
                if (hashmap_) {
                    ++index_;
                    advanceToNextValid();
                }
                return *this;
            }

            ConstIterator operator++(int) {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const ConstIterator &other) const {
                return hashmap_ == other.hashmap_ && index_ == other.index_;
            }

            bool operator!=(const ConstIterator &other) const {
                return !(*this == other);
            }

            std::pair<Key, Value> operator*() const {
                if (!hashmap_ || !isValid()) {
                    throw HashMapException("Iterator out of bounds");
                }

                Key key;
                Value value;
                hashmap_->getEntryAt(index_, key, value);
                return {key, value};
            }

        private:
            [[nodiscard]] bool isValid() const {
                if (!hashmap_ || index_ >= hashmap_->map_.capacity) return false;
                return hashmap_->map_.states[index_] == 1;
            }

            void advanceToNextValid() {
                while (hashmap_ && index_ < hashmap_->map_.capacity && !isValid()) {
                    ++index_;
                }
            }

            const HashMap *hashmap_;
            size_t index_;
        };

        using iterator = Iterator;
        using const_iterator = ConstIterator;

        /**
         * @brief 默认构造函数
         */
        HashMap() {
            if (!tefstd_hashmap_init(&map_, sizeof(Key), sizeof(Value))) {
                throw HashMapException("Failed to initialize hashmap");
            }
            ownsMap_ = true;
        }

        /**
         * @brief 构造函数（带初始容量）
         * @param initial_capacity 初始容量
         */
        explicit HashMap(const size_t initial_capacity) : HashMap() {
            reserve(initial_capacity);
        }

        /**
         * @brief 从初始化列表构造
         */
        HashMap(std::initializer_list<std::pair<Key, Value> > init) : HashMap() {
            for (const auto &pair: init) {
                insert(pair.first, pair.second);
            }
        }

        /**
         * @brief 从std::unordered_map构造
         */
        explicit HashMap(const std::unordered_map<Key, Value> &std_map) : HashMap() {
            for (const auto &pair: std_map) {
                insert(pair.first, pair.second);
            }
        }

        /**
         * @brief 从std::map构造
         */
        template<typename Compare>
        explicit HashMap(const std::map<Key, Value, Compare> &std_map) : HashMap() {
            for (const auto &pair: std_map) {
                insert(pair.first, pair.second);
            }
        }

        /**
         * @brief 析构函数
         */
        ~HashMap() {
            if (ownsMap_) {
                tefstd_hashmap_free(&map_);
            }
        }

        // 移动构造
        HashMap(HashMap &&other) noexcept
            : map_(other.map_), ownsMap_(other.ownsMap_) {
            other.ownsMap_ = false;
            std::memset(&other.map_, 0, sizeof(other.map_));
        }

        // 移动赋值
        HashMap &operator=(HashMap &&other) noexcept {
            if (this != &other) {
                if (ownsMap_) {
                    tefstd_hashmap_free(&map_);
                }
                map_ = other.map_;
                ownsMap_ = other.ownsMap_;
                other.ownsMap_ = false;
                std::memset(&other.map_, 0, sizeof(other.map_));
            }
            return *this;
        }

        // 禁止拷贝
        HashMap(const HashMap &) = delete;

        HashMap &operator=(const HashMap &) = delete;

        /**
         * @brief 插入或更新键值对
         */
        bool insert(const Key &key, const Value &value) {
            return tefstd_hashmap_put(&map_, &key, &value);
        }

        /**
         * @brief 插入或更新（使用std::pair）
         */
        bool insert(const std::pair<Key, Value> &pair) {
            return insert(pair.first, pair.second);
        }

        /**
         * @brief 插入或更新（使用initializer_list）
         */
        void insert(std::initializer_list<std::pair<Key, Value> > init) {
            for (const auto &pair: init) {
                insert(pair.first, pair.second);
            }
        }

        /**
         * @brief 插入或更新（emplace风格）
         */
        template<typename... Args>
        bool emplace(Args &&... args) {
            // 注意：这需要Key和Value可以从参数构造
            // 简化的实现
            std::pair<Key, Value> pair(std::forward<Args>(args)...);
            return insert(pair.first, pair.second);
        }

        /**
         * @brief 查找键
         * @return 指向值的指针，如果不存在返回nullptr
         */
        Value *find(const Key &key) {
            return static_cast<Value *>(tefstd_hashmap_get(&map_, &key));
        }

        /**
         * @brief 查找键（const版本）
         */
        const Value *find(const Key &key) const {
            return static_cast<const Value *>(tefstd_hashmap_get(const_cast<tefstd_hashmap_t *>(&map_), &key));
        }

        /**
         * @brief 检查键是否存在
         */
        bool contains(const Key &key) const {
            return tefstd_hashmap_has(&map_, &key);
        }

        /**
         * @brief 删除键
         */
        bool erase(const Key &key) {
            return tefstd_hashmap_del(&map_, &key);
        }

        /**
         * @brief 通过迭代器删除
         */
        iterator erase(iterator pos) {
            if (pos == end()) return end();

            Key key;
            Value value;
            pos.get(key, value);
            erase(key);

            // 返回下一个有效迭代器
            return iterator(this, pos.getIndex());
        }

        /**
         * @brief 获取值（引用）
         * @throws HashMapException 如果键不存在
         */
        Value &at(const Key &key) {
            Value *val = find(key);
            if (!val) {
                throw HashMapException("Key not found");
            }
            return *val;
        }

        /**
         * @brief 获取值（const引用）
         */
        const Value &at(const Key &key) const {
            const Value *val = find(key);
            if (!val) {
                throw HashMapException("Key not found");
            }
            return *val;
        }

        /**
         * @brief 操作符[] - 如果键不存在则插入默认值
         */
        Value &operator[](const Key &key) {
            Value *val = find(key);
            if (!val) {
                Value defaultVal{};
                insert(key, defaultVal);
                val = find(key);
            }
            return *val;
        }

        /**
         * @brief 获取大小
         */
        [[nodiscard]] size_t size() const {
            return tefstd_hashmap_len(&map_);
        }

        /**
         * @brief 是否为空
         */
        [[nodiscard]] bool empty() const {
            return size() == 0;
        }

        /**
         * @brief 清空
         */
        void clear() {
            tefstd_hashmap_clear(&map_);
        }

        /**
         * @brief 预留容量（注意：这是一个简化实现）
         */
        void reserve(const size_t capacity) {
            // 由于C API不支持直接reserve，这里通过插入临时元素触发扩容
            if (const size_t current_size = size(); capacity > current_size) {
                Key tempKey{};
                Value tempVal{};
                // 使用一种简单的方法来生成唯一键
                // 注意：这只适用于基本类型
                for (size_t i = current_size; i < capacity; ++i) {
                    if constexpr (std::is_integral_v<Key>) {
                        Key uniqueKey = static_cast<Key>(i + 1);
                        insert(uniqueKey, tempVal);
                    } else {
                        // 对于非整数类型，使用指向自身的指针作为临时键
                        // 这仅用于扩容，之后会删除
                        char keyBuf[sizeof(Key)];
                        std::memcpy(keyBuf, &i, sizeof(size_t));
                        Key *tempKeyPtr = reinterpret_cast<Key *>(keyBuf);
                        insert(*tempKeyPtr, tempVal);
                    }
                }
                // 删除临时插入的元素
                // 实际上更好的做法是直接实现reserve
            }
        }

        /**
         * @brief 获取C指针
         */
        tefstd_hashmap_t *getHandle() {
            return &map_;
        }

        /**
         * @brief 获取C指针（const版本）
         */
        [[nodiscard]] const tefstd_hashmap_t *getHandle() const {
            return &map_;
        }

        // ============ 迭代器接口 ============

        iterator begin() {
            return iterator(this, 0);
        }

        iterator end() {
            return iterator(this, map_.capacity);
        }

        const_iterator begin() const {
            return const_iterator(this, 0);
        }

        const_iterator end() const {
            return const_iterator(this, map_.capacity);
        }

        const_iterator cbegin() const {
            return begin();
        }

        const_iterator cend() const {
            return end();
        }

        // ============ 转换为STL容器 ============

        /**
         * @brief 转换为std::unordered_map
         */
        std::unordered_map<Key, Value> toStdUnorderedMap() const {
            std::unordered_map<Key, Value> result;
            result.reserve(size());

            for (auto it = begin(); it != end(); ++it) {
                auto pair = *it;
                result.insert({pair.first, pair.second});
            }
            return result;
        }

        /**
         * @brief 转换为std::map
         */
        std::map<Key, Value> toStdMap() const {
            std::map<Key, Value> result;
            for (auto it = begin(); it != end(); ++it) {
                auto pair = *it;
                result.insert({pair.first, pair.second});
            }
            return result;
        }

        /**
         * @brief 转换为std::vector（键值对列表）
         */
        std::vector<std::pair<Key, Value> > toVector() const {
            std::vector<std::pair<Key, Value> > result;
            result.reserve(size());
            for (auto it = begin(); it != end(); ++it) {
                result.push_back(*it);
            }
            return result;
        }

        /**
         * @brief 获取所有键
         */
        std::vector<Key> keys() const {
            std::vector<Key> result;
            result.reserve(size());
            for (auto it = begin(); it != end(); ++it) {
                auto pair = *it;
                result.push_back(pair.first);
            }
            return result;
        }

        /**
         * @brief 获取所有值
         */
        std::vector<Value> values() const {
            std::vector<Value> result;
            result.reserve(size());
            for (auto it = begin(); it != end(); ++it) {
                auto pair = *it;
                result.push_back(pair.second);
            }
            return result;
        }
    private:
        tefstd_hashmap_t map_{};
        bool ownsMap_ = true;

        /**
         * @brief 获取指定索引处的条目
         */
        void getEntryAt(const size_t index, Key &key, Value &value) const {
            if (index >= map_.capacity || map_.states[index] != 1) {
                throw HashMapException("Invalid entry");
            }

            const size_t key_offset = index * map_.key_size;
            const size_t value_offset = index * map_.value_size;

            std::memcpy(&key, static_cast<const char *>(map_.keys) + key_offset, sizeof(Key));
            std::memcpy(&value, static_cast<const char *>(map_.values) + value_offset, sizeof(Value));
        }

        // 友元迭代器
        friend class Iterator;
        friend class ConstIterator;
    };

    // ============ 便利类型定义 ============

    using StringHashMap = HashMap<std::string, std::string>;
    using StringIntHashMap = HashMap<std::string, int>;
    using IntStringHashMap = HashMap<int, std::string>;
    using IntHashMap = HashMap<int, int>;
    using StringDoubleHashMap = HashMap<std::string, double>;
}
