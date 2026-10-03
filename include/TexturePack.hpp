/*******************************************************************************
 * texturepack_extension - TexturePack
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
 * Created: 2026/6/7
 *******************************************************************************/

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class PackType {
    Terraria = 0, // 标准 ZIP 格式，Content/ 目录
    TEFManager    // TEFManager 格式（预留）
};

struct PackEntry {
    std::string file;
    int priority;
    PackType type;

    PackEntry() : priority(0), type(PackType::Terraria) {
    }
};

/**
 * @brief 单个纹理条目：压缩的原始 PNG 字节 + 完整 assetName（作为 key）
 */
struct TextureEntry {
    std::string assetName;        // 例如 "Content/Images/UI/Foo.png"
    std::vector<uint8_t> data;    // PNG 原始字节（未解码）
};

/**
 * @brief 纹理包类 - 启动时一次性读入全部纹理到内存
 *
 * 加载模型（参考旧版验证过的方案）：
 *   BuildIndex() 打开 ZIP 一次，把所有 Content/ 下的文件解压进 _textures。
 *   之后运行时通过 FindTexture() 直接返回内存指针，不再触碰 ZIP。
 */
class TexturePack {
public:
    explicit TexturePack(PackEntry entry);

    /**
     * @brief 一次性读入 ZIP 中所有纹理到内存（不创建 IL2CPP 对象）
     */
    bool BuildIndex();

    [[nodiscard]] const TextureEntry *FindTexture(const std::string &assetName) const;

    [[nodiscard]] const std::vector<TextureEntry> &GetTextures() const { return _textures; }

    [[nodiscard]] size_t Size() const { return _textures.size(); }

    [[nodiscard]] const std::string &File() const { return _entry.file; }

    void Clear();

private:
    PackEntry _entry;
    std::vector<TextureEntry> _textures;
    std::unordered_map<std::string, size_t> _lookup; // assetName -> index

    bool BuildZipIndex();
};