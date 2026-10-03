/*******************************************************************************
 * File: skipmap.hpp
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

#include "../tefkernel/tefstd/skipmap.h"
#include <map>
#include <vector>
#include <string>
#include <utility>
#include <stdexcept>
#include <iterator>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <functional>

namespace TEFKernel::Std {
    /**
     * @brief 跳表异常类
     */
    class SkipMapException : public std::runtime_error {
    public:
        explicit SkipMapException(const std::string &message)
            : std::runtime_error(message) {
        }
    };

    /**
     * @brief 跳表有序映射包装类
     *
     * 提供类型安全的C++接口，包装C跳表实现。
     * 所有元素按键排序，支持范围查询和有序遍历。
     *
     * @tparam Key 键类型（必须支持比较操作）
     * @tparam Value 值类型
     */
    template<typename Key, typename Value>
    class SkipMap {
    public:
        // STL兼容的类型定义
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const Key, Value>;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        /**
         * @brief 迭代器类（支持STL风格遍历，按顺序）
         */
        class Iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<Key, Value>;
            using difference_type = ptrdiff_t;
            using pointer = value_type *;
            using reference = value_type &;

            Iterator() : skipmap_(nullptr), iter_{nullptr, nullptr, nullptr, false} {
            }

            Iterator(SkipMap *skipmap, const skipmap_iter_t &iter)
                : skipmap_(skipmap), iter_(iter) {
            }

            Iterator &operator++() {
                if (skipmap_) {
                    Key key;
                    Value value;
                    if (tefstd_skipmap_next(&iter_, &key, &value)) {
                        // 迭代器已更新
                    } else {
                        // 到达末尾
                        iter_.current = nullptr;
                    }
                }
                return *this;
            }

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const Iterator &other) const {
                return skipmap_ == other.skipmap_ && iter_.current == other.iter_.current;
            }

            bool operator!=(const Iterator &other) const {
                return !(*this == other);
            }

            std::pair<Key, Value> operator*() const {
                if (!skipmap_ || !iter_.current) {
                    throw SkipMapException("Iterator out of bounds");
                }

                Key key;
                Value value;
                // 获取当前节点的键值
                skipmap_->getNodeValue(iter_.current, key, value);
                return {key, value};
            }

            // 获取键和值的引用（更高效）
            void get(Key &key, Value &value) const {
                if (!skipmap_ || !iter_.current) {
                    throw SkipMapException("Iterator out of bounds");
                }
                skipmap_->getNodeValue(iter_.current, key, value);
            }

        private:
            SkipMap *skipmap_;
            skipmap_iter_t iter_;
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

            ConstIterator() : skipmap_(nullptr), iter_{nullptr, nullptr, nullptr, false} {
            }

            ConstIterator(const SkipMap *skipmap, const skipmap_iter_t &iter)
                : skipmap_(skipmap), iter_(iter) {
            }

            ConstIterator &operator++() {
                if (skipmap_) {
                    Key key;
                    Value value;
                    if (tefstd_skipmap_next(&iter_, &key, &value)) {
                        // 迭代器已更新
                    } else {
                        iter_.current = nullptr;
                    }
                }
                return *this;
            }

            ConstIterator operator++(int) {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const ConstIterator &other) const {
                return skipmap_ == other.skipmap_ && iter_.current == other.iter_.current;
            }

            bool operator!=(const ConstIterator &other) const {
                return !(*this == other);
            }

            std::pair<Key, Value> operator*() const {
                if (!skipmap_ || !iter_.current) {
                    throw SkipMapException("Iterator out of bounds");
                }

                Key key;
                Value value;
                skipmap_->getNodeValue(iter_.current, key, value);
                return {key, value};
            }

        private:
            const SkipMap *skipmap_;
            mutable skipmap_iter_t iter_;
        };

        using iterator = Iterator;
        using const_iterator = ConstIterator;

        /**
         * @brief 默认构造函数
         */
        SkipMap() {
            if (!tefstd_skipmap_init(&map_, sizeof(Key), sizeof(Value))) {
                throw SkipMapException("Failed to initialize skipmap");
            }
            ownsMap_ = true;
        }

        /**
         * @brief 从初始化列表构造
         */
        SkipMap(std::initializer_list<std::pair<Key, Value> > init) : SkipMap() {
            for (const auto &pair: init) {
                insert(pair.first, pair.second);
            }
        }

        /**
         * @brief 从std::map构造（保持有序）
         */
        explicit SkipMap(const std::map<Key, Value> &std_map) : SkipMap() {
            for (const auto &pair: std_map) {
                insert(pair.first, pair.second);
            }
        }

        /**
         * @brief 从std::unordered_map构造
         */
        explicit SkipMap(const std::unordered_map<Key, Value> &unordered_map) : SkipMap() {
            for (const auto &pair: unordered_map) {
                insert(pair.first, pair.second);
            }
        }

        /**
         * @brief 析构函数
         */
        ~SkipMap() {
            if (ownsMap_) {
                tefstd_skipmap_free(&map_);
            }
        }

        // 移动构造
        SkipMap(SkipMap &&other) noexcept
            : map_(other.map_), ownsMap_(other.ownsMap_) {
            other.ownsMap_ = false;
            std::memset(&other.map_, 0, sizeof(other.map_));
        }

        // 移动赋值
        SkipMap &operator=(SkipMap &&other) noexcept {
            if (this != &other) {
                if (ownsMap_) {
                    tefstd_skipmap_free(&map_);
                }
                map_ = other.map_;
                ownsMap_ = other.ownsMap_;
                other.ownsMap_ = false;
                std::memset(&other.map_, 0, sizeof(other.map_));
            }
            return *this;
        }

        // 禁止拷贝
        SkipMap(const SkipMap &) = delete;

        SkipMap &operator=(const SkipMap &) = delete;

        /**
         * @brief 插入或更新键值对
         */
        bool insert(const Key &key, const Value &value) {
            return tefstd_skipmap_put(&map_, &key, &value);
        }

        /**
         * @brief 插入或更新（使用std::pair）
         */
        bool insert(const std::pair<Key, Value> &pair) {
            return insert(pair.first, pair.second);
        }

        /**
         * @brief 批量插入
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
            std::pair<Key, Value> pair(std::forward<Args>(args)...);
            return insert(pair.first, pair.second);
        }

        /**
         * @brief 查找键
         * @return 指向值的指针，如果不存在返回nullptr
         */
        Value *find(const Key &key) {
            return static_cast<Value *>(tefstd_skipmap_get(&map_, &key));
        }

        /**
         * @brief 查找键（const版本）
         */
        const Value *find(const Key &key) const {
            return static_cast<const Value *>(
                tefstd_skipmap_get(const_cast<tefstd_skipmap_t *>(&map_), &key)
            );
        }

        /**
         * @brief 检查键是否存在
         */
        bool contains(const Key &key) const {
            return find(key) != nullptr;
        }

        /**
         * @brief 删除键
         */
        bool erase(const Key &key) {
            return tefstd_skipmap_del(&map_, &key);
        }

        /**
         * @brief 通过迭代器删除
         */
        iterator erase(iterator pos) {
            if (pos == end()) return end();

            Key key;
            Value value;
            pos.get(key, value);
            auto next = std::next(pos);
            erase(key);
            return next;
        }

        /**
         * @brief 获取最小值
         */
        std::optional<std::pair<Key, Value> > min() const {
            if (const void *val = tefstd_skipmap_min(const_cast<tefstd_skipmap_t *>(&map_)); !val)
                return std::nullopt;

            // 获取最小键
            // 注意：我们需要从C跳表中获取最小键
            // 由于API限制，这里简化处理，通过迭代器获取第一个元素
            auto it = begin();
            if (it == end()) {
                return std::nullopt;
            }
            return *it;
        }

        /**
         * @brief 获取最大值
         */
        std::optional<std::pair<Key, Value> > max() const {
            auto it = end();
            // 跳表不支持反向遍历，我们从头遍历到末尾
            // 更高效的方式是使用C API的max函数，但需要获取键
            // 简化实现：遍历到最后一个
            ConstIterator last = begin();
            ConstIterator prev = last;
            while (last != end()) {
                prev = last;
                ++last;
            }
            if (prev == end()) {
                return std::nullopt;
            }
            return *prev;
        }

        /**
         * @brief 获取值（引用）
         * @throws SkipMapException 如果键不存在
         */
        Value &at(const Key &key) {
            Value *val = find(key);
            if (!val) {
                throw SkipMapException("Key not found");
            }
            return *val;
        }

        /**
         * @brief 获取值（const引用）
         */
        const Value &at(const Key &key) const {
            const Value *val = find(key);
            if (!val) {
                throw SkipMapException("Key not found");
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
            return map_.size;
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
            tefstd_skipmap_free(&map_);
            if (!tefstd_skipmap_init(&map_, sizeof(Key), sizeof(Value))) {
                throw SkipMapException("Failed to reinitialize skipmap");
            }
        }

        /**
         * @brief 获取C指针
         */
        tefstd_skipmap_t *getHandle() {
            return &map_;
        }

        /**
         * @brief 获取C指针（const版本）
         */
        [[nodiscard]] const tefstd_skipmap_t *getHandle() const {
            return &map_;
        }

        // ============ 范围查询 ============

        /**
         * @brief 范围查询 - 返回范围内的迭代器
         * @param start 起始键（包含）
         * @param end 结束键（不包含）
         * @return 范围迭代器对
         */
        std::pair<iterator, iterator> range(const Key &start, const Key &end) {
            skipmap_iter_t iter = tefstd_skipmap_range(&map_, &start, &end, false);
            return {iterator(this, iter), end()};
        }

        /**
         * @brief 范围查询（包含结束键）
         */
        std::pair<iterator, iterator> rangeInclusive(const Key &start, const Key &end) {
            skipmap_iter_t iter = tefstd_skipmap_range(&map_, &start, &end, true);
            return {iterator(this, iter), end()};
        }

        /**
         * @brief 范围查询（const版本）
         */
        std::pair<const_iterator, const_iterator> range(const Key &start, const Key &end) const {
            skipmap_iter_t iter = tefstd_skipmap_range(
                const_cast<tefstd_skipmap_t *>(&map_), &start, &end, false
            );
            return {const_iterator(this, iter), end()};
        }

        /**
         * @brief 获取从start开始的所有元素
         */
        std::pair<iterator, iterator> from(const Key &start) {
            return range(start, Key{});
        }

        /**
         * @brief 获取到end为止的所有元素
         */
        std::pair<iterator, iterator> to(const Key &end) {
            skipmap_iter_t iter = tefstd_skipmap_range(&map_, nullptr, &end, false);
            return {iterator(this, iter), end()};
        }

        // ============ 迭代器接口 ============

        iterator begin() {
            skipmap_iter_t iter = tefstd_skipmap_range(&map_, nullptr, nullptr, false);
            return iterator(this, iter);
        }

        iterator end() {
            skipmap_iter_t iter{&map_, nullptr, nullptr, false};
            return iterator(this, iter);
        }

        const_iterator begin() const {
            skipmap_iter_t iter = tefstd_skipmap_range(
                const_cast<tefstd_skipmap_t *>(&map_), nullptr, nullptr, false
            );
            return const_iterator(this, iter);
        }

        const_iterator end() const {
            skipmap_iter_t iter{const_cast<tefstd_skipmap_t *>(&map_), nullptr, nullptr, false};
            return const_iterator(this, iter);
        }

        const_iterator cbegin() const {
            return begin();
        }

        const_iterator cend() const {
            return end();
        }

        // ============ 转换为STL容器 ============

        /**
         * @brief 转换为std::map（保持有序）
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
         * @brief 转换为std::vector（键值对列表，按顺序）
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
         * @brief 获取所有键（按顺序）
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
         * @brief 获取所有值（按键顺序）
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
        tefstd_skipmap_t map_{};
        bool ownsMap_ = true;

        /**
         * @brief 获取节点中的键值
         */
        void getNodeValue(const tefstd_skipnode_t *node, Key &key, Value &value) const {
            if (!node) {
                throw SkipMapException("Invalid node");
            }

            std::memcpy(&key, node->key, sizeof(Key));
            std::memcpy(&value, node->value, sizeof(Value));
        }

        // 友元迭代器
        friend class Iterator;
        friend class ConstIterator;
    };

    // ============ 便利类型定义 ============

    using StringSkipMap = SkipMap<std::string, std::string>;
    using StringIntSkipMap = SkipMap<std::string, int>;
    using IntStringSkipMap = SkipMap<int, std::string>;
    using IntSkipMap = SkipMap<int, int>;
    using StringDoubleSkipMap = SkipMap<std::string, double>;
}