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

#include "TexturePack.hpp"
#include "Log.hpp"
#include "lib/miniz.h"

namespace {
    constexpr auto CONTENT_PREFIX = "Content/";
}

TexturePack::TexturePack(PackEntry entry) : _entry(std::move(entry)) {
}

bool TexturePack::BuildIndex() {
    switch (_entry.type) {
        case PackType::Terraria:
            return BuildZipIndex();
        case PackType::TEFManager:
            LOGW("TEFManager format not yet implemented");
            return false;
        default:
            LOGE("Unknown pack type: %d", static_cast<int>(_entry.type));
            return false;
    }
}

bool TexturePack::BuildZipIndex() {
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_file(&zip, _entry.file.c_str(), 0)) {
        LOGE("Failed to open ZIP: %s", _entry.file.c_str());
        return false;
    }

    const auto file_count = mz_zip_reader_get_num_files(&zip);
    _textures.reserve(static_cast<size_t>(file_count));
    _lookup.reserve(static_cast<size_t>(file_count));

    for (int i = 0; i < file_count; ++i) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, i, &stat)) {
            continue;
        }
        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            continue;
        }

        std::string filename(stat.m_filename);

        // 只索引 Content/ 下的文件
        if (!filename.starts_with(CONTENT_PREFIX)) {
            continue;
        }

        // 剥掉已知扩展名（assetName 不带扩展名）
        for (std::string_view ext : {".png", ".xnb", ".raw", ".jpg", ".jpeg"}) {
            if (filename.ends_with(ext)) {
                filename.erase(filename.size() - ext.size());
                break;
            }
        }

        // 保留先来：同名 key 已存在则跳过（不解压）
        if (_lookup.contains(filename)) {
            continue;
        }

        size_t uncomp_size = 0;
        void *p = mz_zip_reader_extract_file_to_heap(
            &zip, stat.m_filename, &uncomp_size, 0);
        if (!p) {
            LOGE("Extract failed: %s", stat.m_filename);
            continue;
        }

        // 先写 lookup（拷贝 key），再把 filename move 进 _textures
        _lookup[filename] = _textures.size();
        _textures.push_back({
            .assetName = std::move(filename),
            .data = std::vector(static_cast<uint8_t *>(p),
                                static_cast<uint8_t *>(p) + uncomp_size)
        });

        mz_free(p);
    }

    mz_zip_reader_end(&zip);
    LOGI("Indexed %zu textures from %s", _textures.size(), _entry.file.c_str());
    return true;
}

const TextureEntry *TexturePack::FindTexture(const std::string &assetName) const {
    const auto it = _lookup.find(assetName);
    if (it == _lookup.end()) {
        return nullptr;
    }
    return &_textures[it->second];
}

void TexturePack::Clear() {
    _textures.clear();
    _textures.shrink_to_fit();
    _lookup.clear();
}