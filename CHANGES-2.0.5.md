# TexturePackExtension 2.0.5 修改说明

基线：eternalfuture-e38299/TEFManager-TexturePack-Extension（与 lzup333 仓库内容一致）
版本：2.0.0 -> 2.0.5（Info.json versionCode 5 -> 10，module 内部 version_code 11）

## 1. 替换纹理补 BatchTextureIndex

`terraria_texture2d_create()` 走的是 `Texture2D(UnityEngine.Texture2D)` 构造函数，
虽然构造函数末尾会调 `RefreshBatchIndex()`，但独立纹理仍可能与其他纹理
共用批次键，导致 SpriteBatcher 把不同纹理合并到同一批次、只绑定第一张纹理。

修改：对每张替换纹理显式同步

```
BatchTextureIndex = _batchTextureIndex
```

## 2. Fancy 资源集的绘制顺序

`Content/Images/UI/PlayerResourceSets/FancyClassic/*` 是「底色块 + 覆盖填充」
交替绘制：

- 底图：`Heart_Left/Middle/Right/Right_Fancy/Single_Fancy`、`Star_A/B/C/Single`
- 填充：`Heart_Fill/Heart_Fill_B`、`Star_Fill`

问题：替换纹理各自独立批次，按纹理首次出现顺序分组绘制，
后面的底图会盖住前面的填充，表现为满血只有第一颗亮。

处理：

- 底图：`SharedBatching=false` + `NonSharedHeadInsert=true`（head insert 到链前）
- 填充：`SharedBatching=true` + `NonSharedHeadInsert=false`（普通追加到链尾）

最终绘制顺序为「所有底图 -> 所有填充」，每颗心的填充都不会被底图覆盖。

## 3. UI 判定与诊断

- UI 判定扩展为：`/UI/`、`/Menu/`、`/Map/`、`/Backgrounds/`、`/Interface/`、
  `/CharCreation/`、`/WorldCreation/`、`/PlayerSelect/`、`/Cursor/`、
  `/Title/`、`/Logo/`、`/Splash/`、`/Chat/`、`/Emote/`、`/Minimap/`、
  `Inventory_`、`PanelBackground`、`Panel`、`Button`、`Scrollbar`、`TextBox`
- 资源名先做 `\` -> `/` 归一化，兼容不同分隔符
- 看起来像 UI 但没被判成 UI 的，打警告：
  `possible UI texture not classified as UI: <name>`
- 血条相关资源打诊断日志：
  `[heart] name=... batch=... _batch=... shared=... head=...`
- 解码失败打资源名和大小：
  `decode failed: <name> (<bytes>)`

## 4. 非图片文件

索引阶段目前只按扩展名过滤（`.png/.xnb/.raw/.jpg/.jpeg`），
`Content/Localization/*.json` 之类仍会进入解码并失败（日志可见）。
后续可在索引时只保留图片扩展名，避免这些无效解码。

## 5. 构建

见 `build-android-arm64.sh`。
