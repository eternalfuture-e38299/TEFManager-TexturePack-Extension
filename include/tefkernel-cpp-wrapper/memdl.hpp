/*******************************************************************************
 * File: memdl
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
#include <memory>
#include <stdexcept>
#include <vector>

#include "tefkernel/memdl/memdl.h"

namespace TEFKernel {
    class MemdlException : public std::runtime_error {
    public:
        explicit MemdlException(const std::string &message)
            : std::runtime_error(message) {
        }
    };

    /**
     * @brief 内存动态链接库包装类
     *
     * 提供 RAII 风格的内存动态链接库管理，支持从文件或内存数据加载库，
     * 以及符号解析和错误处理。
     */
    class Memdl {
    public:
        // 平台常量
        static constexpr int PlatformUnknown = MEMDL_PLATFORM_UNKNOWN;
        static constexpr int PlatformLinux = MEMDL_PLATFORM_LINUX;
        static constexpr int PlatformAndroid = MEMDL_PLATFORM_ANDROID;
        static constexpr int PlatformMacOS = MEMDL_PLATFORM_MACOS;
        static constexpr int PlatformIOS = MEMDL_PLATFORM_IOS;
        static constexpr int PlatformWindows = MEMDL_PLATFORM_WINDOWS;

        // 架构常量
        static constexpr int ArchUnknown = MEMDL_ARCH_UNKNOWN;
        static constexpr int ArchX86 = MEMDL_ARCH_X86;
        static constexpr int ArchX86_64 = MEMDL_ARCH_X86_64;
        static constexpr int ArchARM = MEMDL_ARCH_ARM;
        static constexpr int ArchARM64 = MEMDL_ARCH_ARM64;

        // 标志常量
        static constexpr int FlagNow = MEMDL_NOW;
        static constexpr int FlagLazy = MEMDL_LAZY;
        static constexpr int FlagLocal = MEMDL_LOCAL;
        static constexpr int FlagGlobal = MEMDL_GLOBAL;

        /**
         * @brief 默认构造函数
         */
        Memdl() : handle_(nullptr) {
        }

        /**
         * @brief 从文件加载动态库
         * @param filename 库文件路径
         * @param flags 加载标志
         * @throws MemdlException 加载失败时抛出
         */
        explicit Memdl(const std::string &filename, const int flags = FlagNow | FlagGlobal) {
            OpenFile(filename, flags);
        }

        /**
         * @brief 从内存数据加载动态库
         * @param data 库数据指针
         * @param size 数据大小
         * @param flags 加载标志
         * @throws MemdlException 加载失败时抛出
         */
        Memdl(const void *data, const size_t size, const int flags = FlagNow | FlagGlobal) {
            OpenMemory(data, size, flags);
        }

        /**
         * @brief 从内存数据加载动态库（使用 vector）
         * @param data 库数据
         * @param flags 加载标志
         * @throws MemdlException 加载失败时抛出
         */
        explicit Memdl(const std::vector<uint8_t> &data, const int flags = FlagNow | FlagGlobal) {
            OpenMemory(data.data(), data.size(), flags);
        }

        /**
         * @brief 移动构造函数
         */
        Memdl(Memdl &&other) noexcept : handle_(other.handle_) {
            other.handle_ = nullptr;
        }

        /**
         * @brief 析构函数 - 自动关闭库
         */
        ~Memdl() {
            Close();
        }

        // 禁止拷贝
        Memdl(const Memdl &) = delete;

        Memdl &operator=(const Memdl &) = delete;

        /**
         * @brief 移动赋值操作符
         */
        Memdl &operator=(Memdl &&other) noexcept {
            if (this != &other) {
                Close();
                handle_ = other.handle_;
                other.handle_ = nullptr;
            }
            return *this;
        }

        /**
         * @brief 从文件打开动态库
         * @param filename 库文件路径
         * @param flags 加载标志
         * @return true 成功，false 失败
         */
        bool OpenFile(const std::string &filename, const int flags = FlagNow | FlagGlobal) {
            Close();
            handle_ = memdl_open_file(filename.c_str(), flags);
            return handle_ != nullptr;
        }

        /**
         * @brief 从内存数据打开动态库
         * @param data 库数据指针
         * @param size 数据大小
         * @param flags 加载标志
         * @return true 成功，false 失败
         */
        bool OpenMemory(const void *data, const size_t size, const int flags = FlagNow | FlagGlobal) {
            Close();
            handle_ = memdl_open(data, size, flags);
            return handle_ != nullptr;
        }

        /**
         * @brief 从内存数据打开动态库（使用 vector）
         * @param data 库数据
         * @param flags 加载标志
         * @return true 成功，false 失败
         */
        bool OpenMemory(const std::vector<uint8_t> &data, const int flags = FlagNow | FlagGlobal) {
            return OpenMemory(data.data(), data.size(), flags);
        }

        /**
         * @brief 解析符号
         * @param symbol 符号名
         * @return 符号地址指针，失败返回 nullptr
         */
        [[nodiscard]] void *GetSymbol(const std::string &symbol) const {
            if (!IsOpen()) {
                return nullptr;
            }
            return memdl_sym(handle_, symbol.c_str());
        }

        /**
         * @brief 解析符号（类型安全版本）
         * @tparam T 函数指针类型
         * @param symbol 符号名
         * @return 类型安全的函数指针，失败返回 nullptr
         */
        template<typename T>
        T GetSymbol(const std::string &symbol) const {
            static_assert(std::is_pointer_v<T>, "T must be a pointer type");
            return reinterpret_cast<T>(GetSymbol(symbol));
        }

        /**
         * @brief 解析符号（带异常版本）
         * @tparam T 函数指针类型
         * @param symbol 符号名
         * @return 类型安全的函数指针
         * @throws MemdlException 符号不存在或库未打开时抛出
         */
        template<typename T>
        T GetSymbolThrows(const std::string &symbol) const {
            static_assert(std::is_pointer_v<T>, "T must be a pointer type");
            if (!IsOpen()) {
                throw MemdlException("Library not open");
            }
            void *addr = GetSymbol(symbol);
            if (!addr) {
                throw MemdlException("Symbol not found: " + symbol);
            }
            return reinterpret_cast<T>(addr);
        }

        /**
         * @brief 关闭动态库
         */
        void Close() {
            if (handle_) {
                memdl_close(handle_);
                handle_ = nullptr;
            }
        }

        /**
         * @brief 检查库是否已打开
         * @return true 已打开，false 未打开
         */
        [[nodiscard]] bool IsOpen() const {
            return handle_ != nullptr;
        }

        /**
         * @brief 获取原生句柄
         * @return 原生句柄
         */
        [[nodiscard]] memdl_handle_t GetHandle() const {
            return handle_;
        }

        /**
         * @brief 获取最后一次错误信息
         * @return 错误信息字符串
         */
        static std::string GetLastError() {
            const char *err = memdl_error();
            return err ? std::string(err) : std::string();
        }

        /**
         * @brief 验证 SO 数据
         * @param data 数据指针
         * @param size 数据大小
         * @return true 有效，false 无效
         */
        static bool Validate(const void *data, const size_t size) {
            return memdl_validate(data, size) != 0;
        }

        /**
         * @brief 验证 SO 数据（使用 vector）
         * @param data 数据
         * @return true 有效，false 无效
         */
        static bool Validate(const std::vector<uint8_t> &data) {
            return Validate(data.data(), data.size());
        }

        /**
         * @brief 获取 SO 的架构
         * @param data 数据指针
         * @param size 数据大小
         * @return 架构常量
         */
        static int GetArch(const void *data, const size_t size) {
            return memdl_get_arch(data, size);
        }

        /**
         * @brief 获取 SO 的架构（使用 vector）
         * @param data 数据
         * @return 架构常量
         */
        static int GetArch(const std::vector<uint8_t> &data) {
            return GetArch(data.data(), data.size());
        }

        /**
         * @brief 获取当前平台
         * @return 平台常量
         */
        static int GetPlatform() {
            return memdl_get_platform();
        }

        /**
         * @brief 检查符号是否存在
         * @param symbol 符号名
         * @return true 存在，false 不存在
         */
        [[nodiscard]] bool HasSymbol(const std::string &symbol) const {
            return GetSymbol(symbol) != nullptr;
        }

    private:
        memdl_handle_t handle_{};
    };

    /**
     * @brief 使用智能指针管理 Memdl 对象
     */
    using MemdlPtr = std::unique_ptr<Memdl>;

    /**
     * @brief 创建 Memdl 对象的工厂函数（从文件）
     */
    inline MemdlPtr MakeMemdl(const std::string &filename, const int flags = Memdl::FlagNow | Memdl::FlagGlobal) {
        auto memdl = std::make_unique<Memdl>();
        if (!memdl->OpenFile(filename, flags)) {
            throw MemdlException("Failed to open library: " + filename +
                                 ", error: " + Memdl::GetLastError());
        }
        return memdl;
    }

    /**
     * @brief 创建 Memdl 对象的工厂函数（从内存）
     */
    inline MemdlPtr MakeMemdl(const void *data, const size_t size, const int flags = Memdl::FlagNow | Memdl::FlagGlobal) {
        auto memdl = std::make_unique<Memdl>();
        if (!memdl->OpenMemory(data, size, flags)) {
            throw MemdlException("Failed to open library from memory, error: " + Memdl::GetLastError());
        }
        return memdl;
    }

    /**
     * @brief 创建 Memdl 对象的工厂函数（从 vector）
     */
    inline MemdlPtr MakeMemdl(const std::vector<uint8_t> &data, const int flags = Memdl::FlagNow | Memdl::FlagGlobal) {
        return MakeMemdl(data.data(), data.size(), flags);
    }
}
