/*******************************************************************************
 * texturepack_extension - TextureManager
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
 * Created: 2026/7/28
 *******************************************************************************/

#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "tefkernel-cpp-wrapper/patchlib/field.hpp"

struct DecodedTexture {
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;
};

/**
 * @brief 纹理管理器 - 全同步，init_module 里一次性搞完
 *
 * 流程（全部阻塞主线程，直到完成）：
 *   1. LoadAllBlocking()  : 16 线程并行解码全部 PNG -> _decoded
 *   2. CommitToIl2Cpp()   : 主线程批量创建 XNA Texture2D -> _textures
 *   3. 运行时 Get()        : 一次哈希查找
 */
class TextureManager final {
public:
    static TextureManager &Instance();

    TextureManager(const TextureManager &) = delete;
    TextureManager &operator=(const TextureManager &) = delete;

    /**
     * @brief 16 线程并行解码全部纹理（阻塞，直到全部完成）
     */
    void LoadAllBlocking();

    void *Get(const std::string &assetName,
                const TEFKernel::PatchLib::Field &sharedBatching,
                const TEFKernel::PatchLib::Field &nonSharedHeadInsert);

    [[nodiscard]] size_t Count() const;

    [[nodiscard]] std::string GetStats() const;

    void Clear();

private:
    TextureManager() = default;
    ~TextureManager() = default;

    std::unordered_map<std::string, DecodedTexture> _decoded;
    std::unordered_map<std::string, void *> _textures;
    mutable std::mutex _texMutex;
};
