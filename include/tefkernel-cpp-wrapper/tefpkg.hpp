/*******************************************************************************
 * tefpackage_cpp_wrapper - tefpkg.hpp
 * Copyright (C) 2026 eternalfuture-e38299
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Author: eternalfuture-e38299
 * GitHub: https://github.com/eternalfuture-e38299
 * Created: 2026/6/28
 *******************************************************************************/

#pragma once

#include "tefkernel/tefpackage/tefpkg.h"
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include <optional>

namespace TEFKernel::TEFPackage {
    /**
     * @brief TEF包异常类
     */
    class TefPkgException : public std::runtime_error {
    public:
        explicit TefPkgException(const std::string &message, const tefpkg_result_t code = TEF_ERROR)
            : std::runtime_error(message), error_code_(code) {
        }

        [[nodiscard]] tefpkg_result_t errorCode() const noexcept { return error_code_; }

    private:
        tefpkg_result_t error_code_;
    };

    /**
     * @brief 压缩类型枚举（C++风格）
     */
    enum class CompressionType : uint8_t {
        None = COMPRESS_NONE,
        LZ4 = COMPRESS_LZ4,
        LZ4HC = COMPRESS_LZ4HC
    };

    /**
     * @brief 访问模式枚举（C++风格）
     */
    enum class AccessMode : uint8_t {
        Closed = TEF_ACCESS_CLOSED,
        Memory = TEF_ACCESS_MEMORY,
        File = TEF_ACCESS_FILE, // 已弃用
        ReadOnly = TEF_ACCESS_READONLY,
        MemData = TEF_ACCESS_MEMDATA,
        ReadWrite = TEF_ACCESS_READWRITE
    };

    /**
     * @brief 条目信息结构体（C++风格）
     */
    struct EntryInfo {
        uint32_t index;
        uint32_t data_offset;
        uint32_t compressed_size;
        uint32_t original_size;
        uint64_t checksum;
        uint64_t timestamp;
        CompressionType compress_type;
        uint8_t compress_level;

        // 从C结构体转换
        explicit EntryInfo(const tefpkg_entry_t *c_entry) {
            if (c_entry) {
                index = c_entry->index;
                data_offset = c_entry->data_offset;
                compressed_size = c_entry->compressed_size;
                original_size = c_entry->original_size;
                checksum = c_entry->checksum;
                timestamp = c_entry->timestamp;
                compress_type = static_cast<CompressionType>(c_entry->compress_type);
                compress_level = c_entry->compress_level;
            }
        }

        // 获取压缩比率
        [[nodiscard]] double compressionRatio() const {
            if (compressed_size == 0) return 1.0;
            return static_cast<double>(original_size) / compressed_size;
        }

        // 是否已压缩
        [[nodiscard]] bool isCompressed() const {
            return compress_type != CompressionType::None;
        }
    };

    /**
     * @brief TEF包管理类（RAII风格）
     *
     * 提供完整的TEF包生命周期管理，支持从文件/内存创建、打开，
     * 添加/提取条目，验证和签名等功能。
     */
    class TefPkg {
    public:
        // 常量
        static constexpr uint32_t Magic = TEFPKG_MAGIC;
        static constexpr uint16_t Version = TEFPKG_VERSION;
        static constexpr uint16_t MaxFiles = TEFPKG_MAX_FILES;

        /**
         * @brief 默认构造函数
         */
        TefPkg() : pkg_(nullptr), own_pkg_(true) {
        }

        /**
         * @brief 从文件创建新的TEF包（预留空间版本）
         * @param filename 文件名
         * @param reserved_entries 预留的条目数量
         * @throws TefPkgException 创建失败时抛出
         */
        TefPkg(const std::string &filename, const uint16_t reserved_entries) {
            tefpkg_t *pkg = nullptr;
            if (const auto result = tefpkg_create_reserved_from_file(filename.c_str(), reserved_entries, &pkg);
                result != TEF_OK || !pkg)
                throw TefPkgException("Failed to create package from file: " + filename, result);
            pkg_ = pkg;
            own_pkg_ = true;
        }

        /**
         * @brief 从内存创建新的TEF包（预留空间版本）
         * @param reserved_entries 预留的条目数量
         * @throws TefPkgException 创建失败时抛出
         */
        explicit TefPkg(const uint16_t reserved_entries) {
            tefpkg_t *pkg = nullptr;
            if (const auto result = tefpkg_create_reserved_from_memory(reserved_entries, &pkg);
                result != TEF_OK || !pkg)
                throw TefPkgException("Failed to create package from memory", result);
            pkg_ = pkg;
            own_pkg_ = true;
        }

        /**
         * @brief 以只读方式打开现有的TEF包
         * @param filename 包文件名
         * @throws TefPkgException 打开失败时抛出
         */
        explicit TefPkg(const std::string &filename) {
            tefpkg_t *pkg = nullptr;
            if (const auto result = tefpkg_open_readonly(filename.c_str(), &pkg); result != TEF_OK || !pkg)
                throw TefPkgException("Failed to open package: " + filename, result);
            pkg_ = pkg;
            own_pkg_ = true;
        }

        /**
         * @brief 从内存数据打开TEF包
         * @param data 内存数据
         * @throws TefPkgException 打开失败时抛出
         */
        explicit TefPkg(const std::vector<uint8_t> &data) {
            tefpkg_t *pkg = nullptr;
            const auto result = tefpkg_open_from_memory(data.data(),
                                                        static_cast<uint32_t>(data.size()),
                                                        &pkg);
            if (result != TEF_OK || !pkg)
                throw TefPkgException("Failed to open package from memory", result);
            pkg_ = pkg;
            own_pkg_ = true;
        }

        /**
         * @brief 使用现有C指针构造（不接管所有权）
         * @param pkg C指针
         */
        explicit TefPkg(tefpkg_t *pkg) : pkg_(pkg), own_pkg_(false) {
        }

        /**
         * @brief 移动构造函数
         */
        TefPkg(TefPkg &&other) noexcept
            : pkg_(other.pkg_), own_pkg_(other.own_pkg_) {
            other.pkg_ = nullptr;
            other.own_pkg_ = false;
        }

        /**
         * @brief 析构函数
         */
        ~TefPkg() {
            Close();
        }

        // 禁止拷贝
        TefPkg(const TefPkg &) = delete;

        TefPkg &operator=(const TefPkg &) = delete;

        /**
         * @brief 移动赋值操作符
         */
        TefPkg &operator=(TefPkg &&other) noexcept {
            if (this != &other) {
                Close();
                pkg_ = other.pkg_;
                own_pkg_ = other.own_pkg_;
                other.pkg_ = nullptr;
                other.own_pkg_ = false;
            }
            return *this;
        }

        /**
         * @brief 保存包到文件
         * @param filename 保存到的文件
         * @param fingerprint 指纹
         * @throws TefPkgException 保存失败时抛出
         */
        void Save(const std::string &filename, const uint64_t fingerprint = 0) const {
            if (!pkg_)
                throw TefPkgException("Package not initialized");
            if (const auto result = tefpkg_save_memory_file(filename.c_str(), pkg_, fingerprint); result != TEF_OK)
                throw TefPkgException("Failed to save package to file: " + filename, result);
        }

        /**
         * @brief 保存包到文件（当前包的文件名）
         * @param fingerprint 指纹
         * @throws TefPkgException 保存失败时抛出
         */
        void Save(const uint64_t fingerprint = 0) const {
            if (!pkg_) {
                throw TefPkgException("Package not initialized");
            }
            if (const auto result = tefpkg_save_file(pkg_, fingerprint); result != TEF_OK)
                throw TefPkgException("Failed to save package", result);
        }

        /**
         * @brief 关闭包并释放资源
         */
        void Close() {
            if (pkg_ && own_pkg_) {
                tefpkg_close(pkg_);
                pkg_ = nullptr;
            }
        }

        /**
         * @brief 添加条目（从内存）
         * @param data 数据
         * @param compress_type 压缩类型
         * @param compress_level 压缩等级
         * @throws TefPkgException 添加失败时抛出
         */
        void AddEntry(const std::vector<uint8_t> &data,
                      CompressionType compress_type = CompressionType::None,
                      const uint8_t compress_level = 0) const {
            if (!pkg_) {
                throw TefPkgException("Package not initialized");
            }
            const auto result = tefpkg_add_entry_from_memory(
                pkg_,
                static_cast<tefpkg_compress_t>(compress_type),
                compress_level,
                const_cast<uint8_t *>(data.data()),
                static_cast<uint32_t>(data.size())
            );
            if (result != TEF_OK) {
                throw TefPkgException("Failed to add entry from memory", result);
            }
        }

        /**
         * @brief 添加条目（从文件）
         * @param filepath 文件路径
         * @param compress_type 压缩类型
         * @param compress_level 压缩等级
         * @throws TefPkgException 添加失败时抛出
         */
        void AddEntryFromFile(const std::string &filepath,
                              CompressionType compress_type = CompressionType::None,
                              const uint8_t compress_level = 0) const {
            if (!pkg_) {
                throw TefPkgException("Package not initialized");
            }
            const auto result = tefpkg_add_entry_from_file(
                pkg_,
                filepath.c_str(),
                static_cast<tefpkg_compress_t>(compress_type),
                compress_level
            );
            if (result != TEF_OK) {
                throw TefPkgException("Failed to add entry from file: " + filepath, result);
            }
        }

        /**
         * @brief 提取条目到内存
         * @param index 条目索引
         * @return 数据向量
         * @throws TefPkgException 提取失败时抛出
         */
        [[nodiscard]] std::vector<uint8_t> ExtractEntry(const uint32_t index) const {
            if (!pkg_) {
                throw TefPkgException("Package not initialized");
            }
            uint8_t *data = nullptr;
            uint32_t data_size = 0;
            if (const auto result = tefpkg_extract_entry_to_memory(pkg_, index, &data, &data_size);
                result != TEF_OK || !data) {
                throw TefPkgException("Failed to extract entry at index: " + std::to_string(index), result);
            }

            // 复制数据到vector并释放C内存
            std::vector result_data(data, data + data_size);
            free(data); // 假设使用malloc分配，需要free
            return result_data;
        }

        /**
         * @brief 提取条目到文件
         * @param index 条目索引
         * @param output_path 输出文件路径
         * @throws TefPkgException 提取失败时抛出
         */
        void ExtractEntryToFile(const uint32_t index, const std::string &output_path) const {
            if (!pkg_)
                throw TefPkgException("Package not initialized");
            if (const auto result = tefpkg_extract_entry_to_file(pkg_, index, output_path.c_str()); result != TEF_OK)
                throw TefPkgException("Failed to extract entry to file: " + output_path, result);
        }

        /**
         * @brief 获取条目信息
         * @param index 条目索引
         * @return 条目信息
         * @throws TefPkgException 获取失败时抛出
         */
        [[nodiscard]] std::optional<EntryInfo> GetEntryInfo(const uint32_t index) const {
            if (!pkg_) {
                throw TefPkgException("Package not initialized");
            }
            tefpkg_entry_t *info = nullptr;
            if (const auto result = tefpkg_get_entry_info(pkg_, index, &info); result != TEF_OK || !info)
                return std::nullopt;
            return EntryInfo(info);
        }

        /**
         * @brief 获取条目数量
         * @return 条目数量
         */
        [[nodiscard]] uint16_t GetEntryCount() const {
            if (!pkg_) {
                return 0;
            }
            return tefpkg_get_entries_count(pkg_);
        }

        /**
         * @brief 获取预留条目数量
         * @return 预留条目数量
         */
        [[nodiscard]] uint16_t GetReservedEntries() const {
            if (!pkg_) {
                return 0;
            }
            return tefpkg_get_reserved_entries(pkg_);
        }

        /**
         * @brief 验证条目完整性
         * @param index 条目索引
         * @return true 验证通过
         * @throws TefPkgException 验证失败时抛出
         */
        [[nodiscard]] bool VerifyEntry(const uint32_t index) const {
            if (!pkg_) {
                throw TefPkgException("Package not initialized");
            }
            const auto result = tefpkg_verify_entry(pkg_, index);
            if (result == TEF_OK) {
                return true;
            }
            if (result == TEF_ERROR_INTEGRITY) {
                return false;
            }
            throw TefPkgException("Failed to verify entry", result);
        }

        /**
         * @brief 验证整个包的完整性
         * @return true 验证通过
         * @throws TefPkgException 验证失败时抛出
         */
        [[nodiscard]] bool VerifyPackage() const {
            if (!pkg_) {
                throw TefPkgException("Package not initialized");
            }
            const auto result = tefpkg_verify_pkg(pkg_);
            if (result == TEF_OK) {
                return true;
            }
            if (result == TEF_ERROR_INTEGRITY) {
                return false;
            }
            throw TefPkgException("Failed to verify package", result);
        }

        /**
         * @brief 验证包签名
         * @param fingerprint 指纹
         * @return true 验证通过
         * @throws TefPkgException 验证失败时抛出
         */
        [[nodiscard]] bool VerifySignature(const uint64_t fingerprint) const {
            if (!pkg_) {
                throw TefPkgException("Package not initialized");
            }
            const auto result = tefpkg_verify_signature(pkg_, fingerprint);
            if (result == TEF_OK) {
                return true;
            }
            if (result == TEF_ERROR_NOT_SIGNATURE || result == TEF_ERROR_INTEGRITY) {
                return false;
            }
            throw TefPkgException("Failed to verify signature", result);
        }

        /**
         * @brief 签名包
         * @param fingerprint 指纹
         * @throws TefPkgException 签名失败时抛出
         */
        void SignPackage(const uint64_t fingerprint) const {
            if (!pkg_) {
                throw TefPkgException("Package not initialized");
            }
            if (const auto result = tefpkg_sign_package(pkg_, fingerprint); result != TEF_OK) {
                throw TefPkgException("Failed to sign package", result);
            }
        }

        /**
         * @brief 获取访问模式
         * @return 访问模式
         */
        [[nodiscard]] AccessMode GetAccessMode() const {
            if (!pkg_) {
                return AccessMode::Closed;
            }
            return static_cast<AccessMode>(pkg_->access_mode);
        }

        /**
         * @brief 检查包是否打开
         * @return true 已打开
         */
        [[nodiscard]] bool IsOpen() const {
            return pkg_ != nullptr;
        }

        /**
         * @brief 获取C指针（不推荐使用）
         * @return C指针
         */
        [[nodiscard]] tefpkg_t *GetHandle() const {
            return pkg_;
        }

        /**
         * @brief 释放所有权（返回C指针）
         * @return C指针
         */
        tefpkg_t *Release() {
            const auto pkg = pkg_;
            pkg_ = nullptr;
            own_pkg_ = false;
            return pkg;
        }

    private:
        tefpkg_t *pkg_;
        bool own_pkg_;
    };

    /**
     * @brief 智能指针类型
     */
    using TefPkgPtr = std::unique_ptr<TefPkg>;

    /**
     * @brief 创建TefPkg智能指针的工厂函数
     */
    inline TefPkgPtr MakeTefPkg(const std::string &filename) {
        return std::make_unique<TefPkg>(filename);
    }

    inline TefPkgPtr MakeTefPkg(const std::string &filename, uint16_t reserved_entries) {
        return std::make_unique<TefPkg>(filename, reserved_entries);
    }

    inline TefPkgPtr MakeTefPkg(const std::vector<uint8_t> &data) {
        return std::make_unique<TefPkg>(data);
    }

    /**
     * @brief 将tefpkg_result_t转换为字符串描述
     */
    inline std::string ResultToString(const tefpkg_result_t result) {
        switch (result) {
            case TEF_OK: return "Success";
            case TEF_ERROR: return "General error";
            case TEF_ERROR_SIGNATURE: return "Signature validation failed";
            case TEF_ERROR_CORRUPT: return "Data corrupted";
            case TEF_ERROR_MEMORY: return "Memory allocation failed";
            case TEF_ERROR_IO: return "I/O error";
            case TEF_ERROR_KEYFILE: return "Key file error";
            case TEF_ERROR_NOT_FOUND: return "Not found";
            case TEF_ERROR_INVALID: return "Invalid parameter or state";
            case TEF_ERROR_NOT_SIGNATURE: return "Not signed";
            case TEF_ERROR_INTEGRITY: return "Integrity check failed";
            case TEF_ERROR_NO_SPACE: return "No space left";
            default: return "Unknown error";
        }
    }
}
