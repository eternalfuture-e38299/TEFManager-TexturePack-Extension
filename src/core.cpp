/*******************************************************************************
 * texturepack_extension - core
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

#include "tefkernel-cpp-wrapper/tefkernel/module/module_core.h"
#include "core.hpp"
#include "TextureManager.hpp"
#include "ThreadPool.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>

#include "Log.hpp"
#include "lib/json.hpp"

#include "tefkernel-cpp-wrapper/patchlib/type.hpp"
#include "tefkernel-cpp-wrapper/patchlib/method.hpp"
#include "tefkernel-cpp-wrapper/patchlib/field.hpp"
#include "tefkernel-cpp-wrapper/patchlib/struct/string.hpp"
#include "tefkernel-cpp-wrapper/tefkernel/terraria/texture2d.h"

static constexpr module_info_t g_module_info = {
    .pkg_id = "eternal.future.texturepackextension",
    .name = "TexturePackExtension",
    .author = "eternalfuture-e38299",
    .version = "2.0.0",
    .version_code = 6,
    .api_version = 1,
    .plugin_dependencies_sizes = 0,
    .plugin_dependencies = nullptr,
};

static TEFKernel::PatchLib::Field FSharedBatching;
static TEFKernel::PatchLib::Field FNonSharedHeadInsert;

// ------- JSON 加载 -------
static bool load_json(const std::filesystem::path &path, std::vector<PackEntry> &output) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOGW("Cannot open %s", path.c_str());
            return false;
        }

        nlohmann::json j;
        file >> j;
        if (!j.is_array()) {
            LOGE("JSON root must be an array");
            return false;
        }

        std::vector<PackEntry> temp;
        for (const auto &item: j) {
            if (item.value("enable", false)) {
                PackEntry entry;
                entry.file = (path.parent_path() / "texture_packs" / item.value("file", "")).string();
                entry.priority = item.value("priority", 0);
                entry.type = static_cast<PackType>(item.value("type", 0));
                temp.push_back(std::move(entry));
            }
        }

        std::ranges::sort(temp, [](const PackEntry &a, const PackEntry &b) {
            return a.priority < b.priority;
        });

        output = std::move(temp);
        return true;
    } catch (const std::exception &e) {
        LOGE("JSON error in %s: %s", path.c_str(), e.what());
        return false;
    }
}

// ------- 材质包并行构建（16 线程，阻塞） -------
static void build_all_packs_parallel(std::vector<PackEntry> &entries) {
    if (entries.empty()) return;

    ThreadPool pool(16);

    std::vector<std::future<std::unique_ptr<TexturePack> > > futures;
    futures.reserve(entries.size());

    for (auto &e: entries) {
        futures.push_back(pool.Enqueue([entry = std::move(e)]() mutable {
            auto pack = std::make_unique<TexturePack>(std::move(entry));
            if (!pack->BuildIndex()) {
                LOGW("BuildIndex failed: %s", pack->File().c_str());
            }
            return pack;
        }));
    }

    for (auto &f: futures) {
        if (auto pack = f.get(); pack && pack->Size() > 0) {
            packs.push_back(std::move(pack));
        }
    }
}

// ------- 全局索引 -------
static void build_global_index() {
    size_t total = 0;
    for (const auto &p: packs) total += p->Size();
    g_texture_indexes.reserve(total * 2);

    for (auto &pack: packs) {
        for (const auto &[assetName, data]: pack->GetTextures()) {
            g_texture_indexes[assetName] = pack.get();
        }
    }
    LOGI("Global index built: %zu entries", g_texture_indexes.size());
}

// ------- LoadTexture2D hook：只剩一次哈希查找 -------
static bool LoadTexture2D_Prefix(patch_handle_t instance, void **args,
                                 const patch_method_signature_t *sig_info, void *result) {
    if (!result) return false;

    const auto assetName = TEFKernel::PatchLib::Struct::String(
        *static_cast<patch_handle_t *>(args[0]), false).ToString();

    if (void *tex = TextureManager::Instance().Get(
        assetName, FSharedBatching, FNonSharedHeadInsert)) {
        *static_cast<patch_handle_t *>(result) = tex;
        return true;
    }
    return false;
}

// ------- 初始化：全部阻塞到底 -------
static bool init_module(module_entry_t *entry) {
    const auto t0 = std::chrono::steady_clock::now();

    // 加载 config
    std::vector<PackEntry> pack_entries;
    if (!load_json(std::filesystem::path(entry->private_dir) / "config.json", pack_entries)) {
        LOGE("Failed to load config.json");
        return false;
    }

    // 并行构建所有 pack（16 线程，阻塞）
    build_all_packs_parallel(pack_entries);

    // 全局索引
    build_global_index();

    // 16 线程并行解码全部 PNG（阻塞，直到完成）
    TextureManager::Instance().LoadAllBlocking();

    // 反射字段（必须在 CommitToIl2Cpp 之前拿到）
    const TEFKernel::PatchLib::Type Texture2d(
        "Microsoft.Xna.Framework.Graphics", "Texture2D");
    FSharedBatching = Texture2d.GetField("SharedBatching");
    FNonSharedHeadInsert = Texture2d.GetField("NonSharedHeadInsert");

    // 安装 hook
    const TEFKernel::PatchLib::Type ContentManager(
        "Microsoft.Xna.Framework.Content", "ContentManager");
    const auto LoadTexture2D = ContentManager.GetMethod("LoadTexture2D", 1);
    patchlib_install_prepost_hook(LoadTexture2D.GetHandle(),
                                  LoadTexture2D_Prefix, nullptr);

    const auto t1 = std::chrono::steady_clock::now();
    LOGI("init_module done in %ld ms",
         std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
    return true;
}

static bool cleanup_module(module_entry_t *entry) {
    /*TextureManager::Instance().Clear();
    packs.clear();
    g_texture_indexes.clear();*/
    return true;
}

static void hot_reload(module_entry_t *entry) {
}

static const module_info_t *get_info() { return &g_module_info; }

static constexpr module_ops_t g_module_ops = {
    .init_module = init_module,
    .cleanup_module = cleanup_module,
    .hot_reload = hot_reload,
    .get_info = get_info,
};

API_EXPORT const module_ops_t * API_CALL module_create(void) {
    return &g_module_ops;
}
