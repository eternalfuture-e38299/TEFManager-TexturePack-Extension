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

#include "TextureManager.hpp"
#include "ThreadPool.hpp"
#include "core.hpp"
#include "Log.hpp"

#include "lib/stb_image.h"
#include "tefkernel-cpp-wrapper/tefkernel/terraria/texture2d.h"

#include <chrono>
#include <ranges>

namespace {
    constexpr size_t kThreads = 16;

    bool isUiTexture(const std::string &assetName) {
        if (assetName.find("PlayerResourceSets") != std::string::npos ||
            assetName.find("HorizontalBars") != std::string::npos) {
            return false;
        }
        return assetName.find("/UI/") != std::string::npos ||
               assetName.find("Inventory_Back") != std::string::npos ||
               assetName.find("PanelBackground") != std::string::npos ||
               assetName.find("/CharCreation/") != std::string::npos ||
               assetName.find("/WorldCreation/") != std::string::npos;
    }

    void fixUi(const std::string &assetName, patch_handle_t texture,
               const TEFKernel::PatchLib::Field &sharedBatching,
               const TEFKernel::PatchLib::Field &nonSharedHeadInsert) {
        if (assetName.find("PlayerResourceSets") != std::string::npos ||
            assetName.find("HorizontalBars") != std::string::npos) {
            sharedBatching.SetValue<bool>(texture, true);
        } else if (isUiTexture(assetName)) {
            sharedBatching.SetValue<bool>(texture, false);
            nonSharedHeadInsert.SetValue<bool>(texture, true);
        } else {
            sharedBatching.SetValue<bool>(texture, true);
        }
    }
}

TextureManager &TextureManager::Instance() {
    static TextureManager instance;
    return instance;
}

void TextureManager::LoadAllBlocking() {
    const auto t0 = std::chrono::steady_clock::now();

    // 收集全部任务
    struct Task {
        std::string assetName;
        const uint8_t *data;
        size_t size;
    };
    std::vector<Task> tasks;
    size_t reserveHint = 0;
    for (const auto &p: packs) reserveHint += p->Size();
    tasks.reserve(reserveHint);

    for (const auto &p: packs) {
        for (const auto &[assetName, data]: p->GetTextures()) {
            tasks.push_back({.assetName = assetName, .data = data.data(), .size = data.size()});
        }
    }

    LOGI("LoadAllBlocking: %zu textures, %zu threads", tasks.size(), kThreads);
    if (tasks.empty()) return;

    // 16 线程并行解码，结果预分配到 vector 槽位（无锁）
    ThreadPool pool(kThreads);

    struct Slot {
        std::string assetName;
        DecodedTexture decoded;
    };
    std::vector<Slot> slots(tasks.size());
    std::vector<std::future<void>> futures;
    futures.reserve(tasks.size());

    for (size_t i = 0; i < tasks.size(); ++i) {
        auto &task = tasks[i];
        auto &slot = slots[i];
        futures.push_back(pool.Enqueue([&task, &slot] {
            slot.assetName = task.assetName;

            int w = 0, h = 0, ch = 0;
            stbi_set_flip_vertically_on_load(true);
            unsigned char *img = stbi_load_from_memory(
                task.data, static_cast<int>(task.size), &w, &h, &ch, 4);
            if (img) {
                slot.decoded.width = w;
                slot.decoded.height = h;
                slot.decoded.pixels.assign(img, img + static_cast<size_t>(w) * h * 4);
                stbi_image_free(img);
            }
        }));
    }

    // 阻塞等待全部完成
    for (auto &f: futures) f.get();

    // 汇总到 _decoded（此时无竞争，单线程写）
    size_t ok = 0, fail = 0;
    _decoded.reserve(tasks.size());
    for (auto &[assetName, decoded]: slots) {
        if (!decoded.pixels.empty()) {
            _decoded[std::move(assetName)] = std::move(decoded);
            ++ok;
        } else {
            ++fail;
        }
    }

    const auto t1 = std::chrono::steady_clock::now();
    LOGI("Decode done: ok=%zu fail=%zu, elapsed=%ld ms",
         ok, fail,
         std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
}

void *TextureManager::Get(const std::string &assetName,
                          const TEFKernel::PatchLib::Field &sharedBatching,
                          const TEFKernel::PatchLib::Field &nonSharedHeadInsert) {
    std::lock_guard lock(_texMutex);

    // 命中缓存
    if (const auto it = _textures.find(assetName); it != _textures.end()) {
        return it->second;
    }

    // 从 _decoded 找像素
    const auto dit = _decoded.find(assetName);
    if (dit == _decoded.end() || dit->second.pixels.empty()) {
        return nullptr;
    }

    // 创建 Texture2D
    auto &[pixels, width, height] = dit->second;
    void *tex = terraria_texture2d_create(
        width, height,
        TEXTURE_FORMAT_RGBA32,
        pixels.data(), pixels.size());
    if (!tex) {
        LOGE("Create Texture2D failed: %s", assetName.c_str());
        return nullptr;
    }

    // 设置 UI 相关字段
    fixUi(assetName, tex, sharedBatching, nonSharedHeadInsert);

    // 缓存 + 释放像素内存
    _textures[assetName] = tex;
    _decoded.erase(dit);

    return tex;
}

size_t TextureManager::Count() const {
    std::lock_guard lock(_texMutex);
    return _textures.size();
}

std::string TextureManager::GetStats() const {
    std::lock_guard lock(_texMutex);
    char buf[96];
    snprintf(buf, sizeof(buf), "Textures: %zu", _textures.size());
    return buf;
}

void TextureManager::Clear() {
    std::lock_guard lock(_texMutex);
    _textures.clear();
    _decoded.clear();
}