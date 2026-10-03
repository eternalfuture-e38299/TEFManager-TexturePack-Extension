/*******************************************************************************
 * File: method.hpp
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

#include "../tefkernel/patchlib/method.h"
#include "type.hpp"
#include "../tefstd/vector.hpp"
#include <string>
#include <functional>
#include <memory>

namespace TEFKernel::PatchLib {
    /**
     * @brief 方法签名信息包装类
     */
    class MethodSignature {
    public:
        MethodSignature() : signature_{} {
            signature_.method = PATCH_NULL;
            signature_.is_instance = false;
            signature_.return_type = PATCH_VOID;
            signature_.arg_types = {};
            signature_.arg_names = {};
            signature_.token = 0;
        }

        explicit MethodSignature(const patch_method_signature_t &sig) : signature_(sig) {
        }

        ~MethodSignature() {
            patchlib_method_signature_free(&signature_);
        }

        // 移动构造
        MethodSignature(MethodSignature &&other) noexcept
            : signature_(other.signature_) {
            std::memset(&other.signature_, 0, sizeof(other.signature_));
        }

        // 移动赋值
        MethodSignature &operator=(MethodSignature &&other) noexcept {
            if (this != &other) {
                patchlib_method_signature_free(&signature_);
                signature_ = other.signature_;
                std::memset(&other.signature_, 0, sizeof(other.signature_));
            }
            return *this;
        }

        // 拷贝被禁用
        MethodSignature(const MethodSignature &) = delete;

        MethodSignature &operator=(const MethodSignature &) = delete;

        [[nodiscard]] patch_handle_t getMethod() const { return signature_.method; }
        [[nodiscard]] bool isInstance() const { return signature_.is_instance; }
        [[nodiscard]] patch_type_t getReturnType() const { return signature_.return_type; }
        [[nodiscard]] int getToken() const { return signature_.token; }

        /**
         * @brief 获取参数类型列表（转换为C++ Vector）
         */
        [[nodiscard]] Std::Vector<patch_type_t> GetArgTypes() const {
            Std::Vector<patch_type_t> result;
            const auto *types = static_cast<const patch_type_t *>(signature_.arg_types.data);
            for (size_t i = 0; i < tefstd_vector_size(&signature_.arg_types); ++i) {
                result.push_back(types[i]);
            }
            return result;
        }

        /**
         * @brief 获取参数名称列表
         */
        [[nodiscard]] Std::Vector<std::string> GetArgNames() const {
            Std::Vector<std::string> result;
            const auto *names = static_cast<const char * const*>(signature_.arg_names.data);
            for (size_t i = 0; i < tefstd_vector_size(&signature_.arg_names); ++i) {
                if (names[i]) {
                    result.push_back(std::string(names[i]));
                }
            }
            return result;
        }

        [[nodiscard]] const patch_method_signature_t *GetRaw() const { return &signature_; }

    private:
        patch_method_signature_t signature_;
    };

    /**
     * @brief Hook ID包装类
     */
    class HookId {
    public:
        HookId() : id_(PATCH_HOOK_INVALID_ID) {
        }

        explicit HookId(const patch_hook_id_t id) : id_(id) {
        }

        [[nodiscard]] patch_hook_id_t Get() const { return id_; }
        [[nodiscard]] bool IsValid() const { return id_ != PATCH_HOOK_INVALID_ID; }

        void Reset() { id_ = PATCH_HOOK_INVALID_ID; }

        bool operator==(const HookId &other) const { return id_ == other.id_; }
        bool operator!=(const HookId &other) const { return id_ != other.id_; }
        explicit operator bool() const { return IsValid(); }

    private:
        patch_hook_id_t id_;
    };

    /**
     * @brief Hook回调函数包装类
     */
    class HookCallbacks {
    public:
        using PrefixCallback = prefix_callback_t;
        using PostfixCallback = postfix_callback_t;

        HookCallbacks() = default;

        HookCallbacks(const PrefixCallback prefix, const PostfixCallback postfix)
            : prefix_(prefix), postfix_(postfix) {
        }

        void SetPrefix(const PrefixCallback callback) { prefix_ = callback; }
        void SetPostfix(const PostfixCallback callback) { postfix_ = callback; }

        prefix_callback_t getPrefix() { return prefix_; }
        postfix_callback_t getPostfix() { return postfix_; }
        [[nodiscard]] const PrefixCallback &getPrefix() const { return prefix_; }
        [[nodiscard]] const PostfixCallback &getPostfix() const { return postfix_; }

    private:
        PrefixCallback prefix_;
        PostfixCallback postfix_;
    };

    /**
     * @brief 方法包装类
     */
    class Method {
    public:
        Method() : handle_(PATCH_NULL), ownsHandle_(false) {
        }

        explicit Method(patch_handle_t handle, const bool takeOwnership = false)
            : handle_(handle), ownsHandle_(takeOwnership) {
        }

        ~Method() {
            if (ownsHandle_ && handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                patchlib_free(handle_);
#endif
            }
        }

        // 拷贝构造
        Method(const Method &other) : handle_(other.handle_), ownsHandle_(false) {
            if (other.ownsHandle_ && other.handle_ != PATCH_NULL) {
#if !defined(__ANDROID__)
                handle_ = patchlib_handle_copy(other.handle_);
                ownsHandle_ = true;
#endif
            }
        }

        // 拷贝赋值
        Method &operator=(const Method &other) {
            if (this != &other) {
                reset();
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
        Method(Method &&other) noexcept
            : handle_(other.handle_), ownsHandle_(other.ownsHandle_) {
            other.handle_ = PATCH_NULL;
            other.ownsHandle_ = false;
        }

        // 移动赋值
        Method &operator=(Method &&other) noexcept {
            if (this != &other) {
                reset();
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

        void reset(patch_handle_t newHandle = PATCH_NULL, const bool takeOwnership = false) {
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
            const char *name = patchlib_method_get_name(handle_);
            return name ? std::string(name) : std::string();
        }

        [[nodiscard]] int GetParamCount() const {
            return patchlib_method_get_param_count(handle_);
        }

        [[nodiscard]] bool IsInstance() const {
            return patchlib_method_is_instance(handle_);
        }

        [[nodiscard]] bool IsStatic() const {
            return patchlib_method_is_static(handle_);
        }

        [[nodiscard]] int GetToken() const {
            return patchlib_method_get_token(handle_);
        }

        [[nodiscard]] patch_type_t GetReturnType() const {
            return patchlib_method_get_return_type(handle_);
        }

        // ==================== 泛型 ====================

        [[nodiscard]] Method MakeGenericInstance(const Std::Vector<patch_handle_t> &templateTypes) const {
            return Method(
                patchlib_method_make_generic_instance(
                    handle_,
                    templateTypes.getHandle()
                ),
                true
            );
        }

        // ==================== 调用 ====================

#if __ANDROID__
        void *GetPointer() const {
            return patchlib_method_get_pointer(handle_);
        }
#endif

        /**
         * @brief 调用方法（模板版本）
         */
        template<typename ReturnType, typename... Args>
        ReturnType Invoke(patch_handle_t instance, Args... args) const {
            void *argArray[] = {&args...};
            ReturnType result;
            patchlib_method_invoke_args(handle_, instance, &result, argArray);
            return result;
        }

        /**
         * @brief 调用方法（void返回类型）
         */
        template<typename... Args>
        void InvokeVoid(patch_handle_t instance, Args... args) const {
            void *argArray[] = {&args...};
            patchlib_method_invoke_args(handle_, instance, nullptr, argArray);
        }

        /**
         * @brief 调用方法（原始指针版本）
         */
        bool InvokeArgs(patch_handle_t instance, void *returnValue, void **args) const {
            return patchlib_method_invoke_args(handle_, instance, returnValue, args);
        }

        /**
         * @brief 调用构造函数并创建新实例
         * @param args 构造函数的参数
         * @return 新创建的实例句柄，失败返回PATCH_NULL
         */
        template<typename... Args>
        patch_handle_t InvokeConstructor(Args... args) const {
            void* argArray[] = {&args...};
            patch_handle_t instance = PATCH_NULL;

            if (patchlib_constructor_invoke(handle_, &instance, argArray)) {
                return instance;
            }
            return PATCH_NULL;
        }

        /**
         * @brief 调用构造函数并返回包装的Type对象
         */
        template<typename... Args>
        Type InvokeConstructorAsType(Args... args) const {
            patch_handle_t instance = InvokeConstructor(args...);
            if (instance != PATCH_NULL) {
                return Type(instance, true); // 拥有所有权
            }
            return {}; // 返回无效Type
        }

        /**
         * @brief 调用构造函数（原始指针版本）
         * @param return_instance 输出新实例句柄
         * @param args 参数指针数组
         * @return 是否成功
         */
        bool InvokeConstructorRaw(patch_handle_t* return_instance, void** args) const {
            if (return_instance == nullptr) {
                return false;
            }
            return patchlib_constructor_invoke(handle_, return_instance, args);
        }

        // ==================== 签名 ====================

        [[nodiscard]] std::unique_ptr<MethodSignature> GetSignature() const {
            if (auto sig = std::make_unique<MethodSignature>(); patchlib_method_get_signature(handle_, const_cast<patch_method_signature_t *>(sig->GetRaw()))) {
                return sig;
            }
            return nullptr;
        }

        // ==================== Hook ====================

        /**
         * @brief 安装Prefix/Postfix Hook
         */
        [[nodiscard]] HookId InstallPrePostHook(
            HookCallbacks::PrefixCallback prefix,
            HookCallbacks::PostfixCallback postfix
        ) const {
            const auto callbacks = std::make_shared<HookCallbacks>(prefix, postfix);

            const patch_hook_id_t id = patchlib_install_prepost_hook(
                handle_,
                callbacks.get()->getPrefix(), // 实际应传入prefixWrapper
                callbacks.get()->getPostfix() // 实际应传入postfixWrapper
            );

            return HookId(id);
        }

        /**
         * @brief 卸载Hook
         */
        static bool UninstallHook(const HookId &hookId) {
            return patchlib_uninstall_hook(hookId.Get());
        }

        // ==================== 运算符 ====================

        explicit operator bool() const {
            return IsValid();
        }

        bool operator==(const Method &other) const {
            return handle_ == other.handle_;
        }

        bool operator!=(const Method &other) const {
            return !(*this == other);
        }

    private:
        patch_handle_t handle_;
        bool ownsHandle_;
    };

    // Type类中需要的方法实现
    inline Method Type::GetMethod(const std::string &name) const {
        return Method(patchlib_type_get_method(handle_, name.c_str()), false);
    }

    inline Method Type::GetMethod(const std::string &name, int argsCount) const {
        return Method(patchlib_type_get_method_by_param_count(handle_, name.c_str(), argsCount), false);
    }

    inline Method Type::GetMethod(
        const std::string &name,
        const Std::Vector<std::string> &argsNames
    ) const {
        // 构建C字符串数组
        std::vector<const char *> cStrings;
        cStrings.reserve(argsNames.size());
        for (const auto &s: argsNames) {
            cStrings.push_back(s.c_str());
        }
        return Method(
            patchlib_type_get_method_by_param_names(
                handle_,
                name.c_str(),
                static_cast<int>(argsNames.size()),
                cStrings.data()
            ),
            false
        );
    }

    inline Method Type::GetMethod(
        const std::string &name,
        const Std::Vector<Type> &argsTypes
    ) const {
        // 构建句柄数组
        Std::Vector<patch_handle_t> handles;
        handles.reserve(argsTypes.size());
        for (const auto &type: argsTypes) {
            handles.push_back(type.GetHandle());
        }
        return Method(
            patchlib_type_get_method_by_param_types(
                handle_,
                name.c_str(),
                static_cast<int>(argsTypes.size()),
                handles.data()
            ),
            false
        );
    }

    inline Method Type::GetMethod(
        const std::string &name,
        const Std::Vector<Type> &argsTypes,
        const Std::Vector<std::string> &argsNames
    ) const {
        // 构建句柄数组
        Std::Vector<patch_handle_t> handles;
        handles.reserve(argsTypes.size());
        for (const auto &type: argsTypes) {
            handles.push_back(type.GetHandle());
        }

        // 构建C字符串数组
        std::vector<const char *> cStrings;
        cStrings.reserve(argsNames.size());
        for (const auto &s: argsNames) {
            cStrings.push_back(s.c_str());
        }

        return Method(
            patchlib_type_get_method_by_signature(
                handle_,
                name.c_str(),
                static_cast<int>(argsTypes.size()),
                handles.data(),
                cStrings.data()
            ),
            false
        );
    }

    inline Std::Vector<Method> Type::GetMethods(const bool includingParent) const {
        tefstd_vector_t vec;
        if (!tefstd_vector_init(&vec, sizeof(patch_handle_t))) {
            return {};
        }

        if (patchlib_type_get_methods(handle_, includingParent, &vec)) {
            Std::Vector<Method> result;
            const auto *data = static_cast<patch_handle_t *>(vec.data);
            for (size_t i = 0; i < vec.size; ++i) {
                result.push_back(Method(data[i], false));
            }
            tefstd_vector_destroy(&vec);
            return result;
        }

        tefstd_vector_destroy(&vec);
        return {};
    }
}