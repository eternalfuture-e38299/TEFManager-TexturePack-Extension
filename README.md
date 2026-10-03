# TEFManager-TexturePack-Extension

> TEFManager 材质包扩展模块  
> 基于 TEFKernel 框架的 Terraria 安卓平台材质包加载系统  

---

## 📖 项目介绍

TEFManager 材质包扩展模块，为 Terraria 提供自定义纹理加载能力，支持标准格式材质包，实现纹理的异步预加载和智能缓存管理。

### ✨ 核心特性

- **🎨 纹理替换** - 支持游戏内任意纹理的替换（UI、物品、NPC、弹幕等）
- **📦 多格式支持** - 兼容标准 Terraria 格式材质包
- **⚡ 异步加载** - 启动时自动预加载，不影响游戏流畅度
- **🧠 智能缓存** - 已加载纹理自动缓存，避免重复解码
- **🔄 取消机制** - 支持取消正在进行的异步加载任务

### 支持格式

| 类型         | 说明       | 目录结构                     |
|--------------|------------|------------------------------|
| `Terraria`   | 标准格式   | `Content/` 目录              |
| `TEFManager` | 预留格式   | 暂未实现                     |

---

## 📦 材质包制作指南

### 目录结构

#### 标准格式 (Terraria)

```text
材质包名称.zip
└── Content/
    ├── Images/
    │   ├── UI/
    │   │   ├── Inventory_Back.png
    │   │   └── PanelBackground.png
    │   ├── NPC/
    │   │   └── NPC_Head_1.png
    │   └── Tiles/
    │       └── Tile_1.png
    └── Map/
        └── MapBG.png
```

---

## 🎨 纹理规范

### 图片格式要求

- **格式**：PNG（32bit RGBA）
- **尺寸**：建议为 2 的幂次方（如 64x64、128x128、256x256）
- **色彩**：支持透明通道（Alpha）

### 常用纹理路径

| 路径                                         | 说明     |
|----------------------------------------------|----------|
| `Content/Images/UI/Inventory_Back`           | 背包背景 |
| `Content/Images/UI/PanelBackground`          | 面板背景 |
| `Content/Images/NPC/NPC_Head_{id}`           | NPC 头像 |
| `Content/Images/Tiles/Tile_{id}`             | 图块纹理 |
| `Content/Images/Projectiles/Projectile_{id}` | 弹幕纹理 |
| `Content/Images/Map/MapBG`                   | 地图背景 |

### 优先级规则

1. **覆盖顺序**：高优先级材质包会覆盖低优先级的同路径纹理
2. **索引构建**：按优先级排序后建立索引，同名纹理只保留高优先级版本
3. **回退机制**：缺失的纹理自动使用原版纹理

## ⚠️ 注意事项

1. **纹理尺寸**：建议使用 2 的幂次方尺寸，避免性能问题
2. **内存管理**：大尺寸纹理（如 2048x2048）会消耗大量内存，请合理控制
3. **格式兼容**：仅支持 PNG 格式，不支持 JPG 等其他格式
4. **异步加载**：`Get()` 方法会检测异步状态，未完成则取消并同步加载
5. **UI 批次**：UI 纹理自动设置 `SharedBatching=0` 以修复显示问题

---

## ❓ 常见问题

**Q: 纹理加载后显示不正确？**
A: 检查纹理尺寸是否为 2 的幂次方，以及 PNG 格式是否正确。

**Q: 如何调试纹理加载问题？**
A: 查看日志输出，`LOGD` 和 `LOGE` 会输出详细的加载信息。

**Q: 多个材质包如何覆盖？**
A: 通过 `priority` 控制优先级，数值越小优先级越高。

**Q: 材质包加载失败怎么办？**
A: 检查 ZIP 文件完整性，确认目录结构符合要求。

## 🤝 贡献指南

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

---

## 📄 许可证
本项目采用 GNU Affero General Public License v3.0 许可证。
---

## 🔗 相关链接

- [TEFKernel](https://github.com/eternalfuture-e38299/TEFKernel) - 核心框架
- [问题反馈](https://github.com/eternalfuture-e38299/TEFManager-TexturePack-Extension/issues)
