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

#include <fstream>
#include <vector>

#include "Log.hpp"
#include "TexturePack.hpp"
#include "lib/json.hpp"

#include "tefkernel-cpp-wrapper/patchlib/type.hpp"
#include "tefkernel-cpp-wrapper/patchlib/method.hpp"
#include "tefkernel-cpp-wrapper/patchlib/field.hpp"
#include "tefkernel-cpp-wrapper/patchlib/struct/string.hpp"
#include "tefkernel-cpp-wrapper/tefkernel/terraria/texture2d.h"


static constexpr module_info_t g_module_info = {
    .pkg_id = "eternal.future.texturepackextension", // 唯一包名
    .name = "TexturePackExtension", // 插件名称
    .author = "eternalfuture-e38299", // 作者
    .version = "1.0.3", // 版本
    .version_code = 4, // 版本代码
    .api_version = 1, // API版本
    .plugin_dependencies_sizes = 0, // 依赖插件数组大小（如需依赖请修改）
    .plugin_dependencies = nullptr, // 依赖插件列表
};

static bool load_json(const std::filesystem::path &path, std::vector<PackEntry> &output) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOGW("Warning: Cannot open %s\n", path.c_str());
            return false;
        }

        nlohmann::json j;
        file >> j;

        if (!j.is_array()) {
            LOGE("Error: JSON root must be an array\n");
            return false;
        }

        // 临时存储所有 enable=true 的条目
        std::vector<PackEntry> temp;

        for (const auto &item: j) {
            if (item.value("enable", false)) {
                PackEntry entry;
                entry.file = path.parent_path() / "texture_packs" / item.value("file", "");
                entry.priority = item.value("priority", 0);
                entry.type = static_cast<PackType>(item.value("type", 0));
                temp.push_back(entry);
            }
        }

        // 按优先级排序
        std::ranges::sort(temp,
                          [](const PackEntry &a, const PackEntry &b) {
                              return a.priority < b.priority;
                          });

        output = std::move(temp); // 移动赋值，高效
        return true;
    } catch (const nlohmann::json::parse_error &e) {
        LOGE("JSON parse error in %s: %s\n", path.c_str(), e.what());
        return false;
    } catch (const std::exception &e) {
        LOGE("Error reading %s: %s\n", path.c_str(), e.what());
        return false;
    }
}

// 纹理批次字段（决定绘制顺序，UI 面板需非共享头插）
static TEFKernel::PatchLib::Field FSharedBatching;
static TEFKernel::PatchLib::Field FNonSharedHeadInsert;

static bool isUiTexture(const std::string& assetName) {
    // 血条/资源条需要单独处理
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

static void fixUi(const std::string& assetName, patch_handle_t texture) {
    // 批次字段处理（规律明确，无需参考原版对象）：
    // UI 面板类纹理需 SharedBatching=0 + NonSharedHeadInsert=1（非共享分支头插，先画背景，
    // 避免尾插盖住上层文字/图标）。物品/方块/弹幕等世界内容 SharedBatching=1。

    // 血条/资源条类纹理需要SharedBatching=1（共享批处理）
    if (assetName.find("PlayerResourceSets") != std::string::npos ||
        assetName.find("HorizontalBars") != std::string::npos) {
        FSharedBatching.SetValue<bool>(texture, true);
        LOGD("LoadTexture2D: health bar texture, SharedBatching=1 for %s", assetName.c_str());
    } else if (isUiTexture(assetName)) {
        FSharedBatching.SetValue<bool>(texture, false);
        FNonSharedHeadInsert.SetValue<bool>(texture, true);
        LOGD("LoadTexture2D: UI texture, SharedBatching=0 NonShared=1 for %s", assetName.c_str());
    } else {
        FSharedBatching.SetValue<bool>(texture, true);
        LOGD("LoadTexture2D: world texture, keep SharedBatching=1 for %s", assetName.c_str());
    }
}

// Prefix：若 assetName 在材质包索引中，直接创建材质包纹理并作为返回值
// （返回 true 跳过原方法，避免先加载原版再加载材质包造成双重加载）
static bool LoadTexture2D_Prefix(patch_handle_t instance, void **args,
                                 const patch_method_signature_t *sig_info, void *result) {
    if (!result) {
        return false;
    }

    const auto assetName = TEFKernel::PatchLib::Struct::String(*static_cast<patch_handle_t *>(args[0]), false).
            ToString();

    // 不在材质包索引中则使用原版纹理（走原方法）
    if (!g_texture_indexes.contains(assetName)) {
        return false;
    }

    // 获取材质包纹理数据
    TextureData textureData = TextureManager::Instance().Get(assetName);
    if (!textureData.IsValid()) {
        LOGW("LoadTexture2D: invalid texture data: %s", assetName.c_str());
        return false;
    }

    // 创建材质包的新纹理（UnityEngine 像素）
    const auto texture = terraria_texture2d_create(
        textureData.width,
        textureData.height,
        TEXTURE_FORMAT_RGBA32,
        textureData.data.data(),
        textureData.data.size()
    );
    if (!texture) {
        LOGE("LoadTexture2D: failed to create texture: %s", assetName.c_str());
        return false;
    }

    fixUi(assetName, texture);

    // 直接作为返回值（跳过原方法，只加载材质包一份）
    *static_cast<patch_handle_t *>(result) = texture;
    LOGD("LoadTexture2D: replaced texture object for %s", assetName.c_str());
    return true;
}

/*
// Postfix：修复Ui显示问题
static void LoadTexture2D_Postfix(patch_handle_t instance, void **args,
                                  void *result, const patch_method_signature_t *sig_info) {
    const auto texture = *static_cast<patch_handle_t*>(instance);
    if (!texture) return;


    const auto assetName = TEFKernel::PatchLib::Struct::String(*static_cast<patch_handle_t *>(args[0]), false).
            ToString();

// 08-18 20:35:24.302  8598  8630 I texturepack_extension: load Content/Images/UI/PageIcons/MapSelected

    if (assetName.find("PanelBackground") != std::string::npos) {
        // 修复物品的 UI 显示问题
        FSharedBatching.SetValue<bool>(texture, false);
        FNonSharedHeadInsert.SetValue<bool>(texture, true);
        LOGD("LoadTexture2D: Item texture set to UI mode for %s", assetName.c_str());
    }
}
*/

/**
 * @brief 初始化模块
 * @param entry 模块条目指针
 * @return true-成功, false-失败
 */
static bool init_module(module_entry_t *entry) {
    std::vector<PackEntry> pack_entries{};

    load_json(std::filesystem::path(entry->private_dir) / "config.json", pack_entries);

    // 预先分配容量，避免 push_back 扩容导致 g_texture_indexes 中已存储的指针失效（悬空）
    packs.reserve(pack_entries.size());
    g_texture_indexes.reserve(pack_entries.size());

    for (const auto &_pack: pack_entries) {
        TexturePack pack(_pack);
        pack.BuildIndex();
        packs.push_back(pack); // 拷贝到packs中

        for (auto entries_names = pack.GetEntryNames(); const auto &entry_name: entries_names) {
            // 存储packs中最后一个元素的地址（即刚push_back的元素）
            g_texture_indexes[entry_name] = &packs.back();
        }
    }

    TextureManager::Instance().LoadAllAsync();

    const TEFKernel::PatchLib::Type ContentManager("Microsoft.Xna.Framework.Content", "ContentManager");
    const auto LoadTexture2D = ContentManager.GetMethod("LoadTexture2D", 1);

    const TEFKernel::PatchLib::Type Texture2d("Microsoft.Xna.Framework.Graphics", "Texture2D");
    FSharedBatching = Texture2d.GetField("SharedBatching");
    FNonSharedHeadInsert = Texture2d.GetField("NonSharedHeadInsert");

    // ReSharper disable once CppNoDiscardExpression
    patchlib_install_prepost_hook(LoadTexture2D.GetHandle(), LoadTexture2D_Prefix, nullptr);

    return true;
}

/**
 * @brief 清理并关闭模块
 * @param entry 模块条目指针
 * @return true-成功, false-失败
 */
static bool cleanup_module(module_entry_t *entry) {
    return true;
}

/**
 * @brief 热重载操作
 * @param entry 模块条目指针
 */
static void hot_reload(module_entry_t *entry) {
}

static const module_info_t *get_info() {
    return &g_module_info;
}

static constexpr module_ops_t g_module_ops = {
    .init_module = init_module,
    .cleanup_module = cleanup_module,
    .hot_reload = hot_reload,
    .get_info = get_info,
};

API_EXPORT const module_ops_t * API_CALL module_create(void) {
    return &g_module_ops;
}
