# TypingTrainer

一个使用 **C++17 / Qt6** 编写的中文打字练习软件，专注于**五笔等形码输入法**的跟打训练。

支持吃豆人式跟打、双行对照跟打、码表提示、测速点与速度曲线、错字统计与跳转、
历史成绩记录、背景音乐、可自定义主题等功能。

---

## 功能特性

- **跟打模式**
  - **吃豆人模式**：打对一个字，该字立即变灰（被"吃掉"），当前字高亮反色，打错的字可选标红。
  - **双行对照模式**：上行显示原文，下行实时显示用户输入字符，逐字对照。
- **码表支持**
  - 内置 **五笔 86 / 五笔 98 / 郑码** 码表，可导入自定义码表。
  - 支持两种码表格式，自动识别：
    - `汉字<Tab>编码1 编码2 ...`
    - `编码<Tab>汉字`
  - 打错时底部浮动提示条显示当前字/词的编码与候选词（2 秒后自动隐藏）。
- **文本处理**
  - 自动检测编码（UTF-8 BOM / UTF-8 / GBK / Latin-1 兜底）。
  - 文本过滤：过滤汉字、非汉字、大写、小写、数字、空格；大小写转换。
  - 乱序模式：去除标点后随机打乱字符顺序，适合单字/词组练习。
  - 文本库：内置古诗词、散文等范文，可导入/删除/重命名本地文本。
- **统计与测速**
  - 实时状态栏：进度、速度（字/分）、击键（次/秒）、码长、错字数、回改次数。
  - 测速点：可自动识别文本中的标记点，或按位置/时间间隔记录速度快照。
  - **速度曲线图**（基于 Qt Charts），可从图表重打某一段。
- **错字管理**：记录每个错字的期望字符、实际输入与累计次数，支持跳转到错字位置。
- **历史成绩**：SQLite 数据库记录每次练习的时长、速度、正确率等，支持查询/过滤/导出/删除。
- **断点续打**：记住每篇文章的上次位置，可从头 / 随机 / 断点开始。
- **主题与字体**：跟随系统 / 亮色 / 暗色，支持逐角色自定义颜色；打字字体可配置。
- **背景音乐**：播放列表、上一首/下一首、循环模式、音量，启动自动播放，状态持久化。
- **倒计时**：可设置练习倒计时提醒。

---

## 构建

### 依赖

- CMake ≥ 3.16
- 支持 C++17 的编译器（GCC / Clang / MSVC）
- Qt6，组件：`Widgets` `Core` `Gui` `Charts` `Multimedia` `Sql`

> `Charts`（速度曲线）与 `Sql`（历史记录）为可选，缺失时对应功能编译期自动禁用
> （通过 `HAVE_QTCHARTS` / `HAVE_QTSQL` 宏控制）。

### 编译步骤

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/TypingTrainer
```

Windows 生成的是 GUI 程序（`WIN32_EXECUTABLE`），macOS 生成 `.app` 包。

---

## 目录结构

```
├── CMakeLists.txt
├── resources/
│   ├── resources.qrc          # 内置资源清单
│   ├── tables/                # 内置码表 (wubi86/wubi98/zhengma)
│   └── texts/                 # 内置范文
└── src/
    ├── main.cpp               # 程序入口
    ├── app/                   # 应用层
    │   ├── Application.*      # QApplication 子类，负责配置/主题初始化
    │   ├── ConfigManager.*    # JSON 配置读写（单例）
    │   └── MusicPlayer.*      # 背景音乐播放器
    ├── core/                  # 核心业务逻辑（与 UI 解耦）
    │   ├── TextDocument.*     # 文本模型 + 段落解析
    │   ├── TypingSession.*    # 打字会话与统计（核心状态机）
    │   ├── TextShuffler.*     # 文本去标点/乱序
    │   ├── TextFilter.*       # 文本过滤与大小写转换
    │   ├── CodeTable.*        # 码表加载与查询
    │   ├── SpeedPointFinder.* # 测速点自动识别
    │   └── HistoryDb.*        # SQLite 历史成绩库（单例）
    ├── theme/
    │   └── ThemeManager.*     # 主题/调色板管理（单例）
    ├── ui/                    # 界面层
    │   ├── MainWindow.*       # 主窗口，装配控制器与视图
    │   ├── TypingView.*       # 打字视图抽象基类（键盘/输入法/焦点）
    │   ├── WrappedTextView.*  # 折行布局视图基类
    │   ├── PacmanView.*       # 吃豆人模式视图
    │   ├── TwoLineView.*      # 双行对照模式视图
    │   ├── CodeHintPanel.*    # 编码浮动提示条
    │   ├── SettingsDialog.*   # 设置对话框（分页）
    │   ├── HistoryDialog.*    # 历史成绩
    │   ├── MistakeDialog.*    # 错字列表
    │   ├── TextLibraryDialog.*# 文本库
    │   ├── MusicLibraryDialog.*# 音乐库
    │   ├── SpeedPointDialog.* # 测速点设置
    │   ├── SpeedChartDialog.* # 速度曲线
    │   ├── settings/          # 设置各分页（字体/主题/启动/过滤）
    │   └── controllers/       # 控制器（菜单、音乐、文本、码表、测速、主题、历史）
    └── utils/
        ├── AppPaths.*         # 应用目录 / 文件路径
        └── TextLoader.*       # 编码检测与文件读写
```

### 架构说明

项目采用**控制器 + 视图**的轻量分层设计：

- `MainWindow` 只负责**装配**：创建控制器、视图与会话，连接信号槽，不承载具体业务。
- `controllers/` 中的控制器各司其职（加载文本、管理码表、测速、主题、历史、音乐、
  菜单构建），通过信号与宿主通信，避免相互耦合。
- `core/` 为纯业务逻辑，不依赖 UI，便于测试与复用。
- `TypingView` 定义打字视图接口；`WrappedTextView` 实现软折行布局、滚动与裁剪，
  具体模式（吃豆人 / 双行对照）只需实现每行高度与绘制两个钩子。

---

## 数据与配置路径

由 `AppPaths` 统一管理（遵循各平台惯例，基于 `QStandardPaths`）：

| 用途 | 说明 |
| --- | --- |
| 配置 | `configDir()` → `configFilePath()`（JSON，`ConfigManager` 读写） |
| 数据 | `dataDir()` |
| 码表 | `codeTableDir()` |
| 文本 | `textDir()` |
| 历史 | `historyDbPath()`（SQLite，`HistoryDb` 读写） |

> 注：程序**未设置** `organizationName`，以避免 Linux 下配置文件出现
> 双层嵌套目录（详见 `Application.cpp`）。

---

## 常用快捷键

| 快捷键 | 功能 |
| --- | --- |
| `F3` | 重打当前段 |
| `Ctrl+M` | 查看错字列表（可跳转到错字位置） |

---

## 码表格式

主流格式（每行一条，`Tab` 分隔）：

```
工	aaaa
式	aad
```

也兼容旧格式 `编码<Tab>汉字`。加载时通过行首是否为汉字自动识别格式。

---

## 内置资源

- **码表**：五笔 86、五笔 98、郑码。
- **范文**：常用字（前/中/后 500）、岳阳楼记、背影、断章、沙扬娜拉、致橡树、
  面朝大海春暖花开、雨巷、再别康桥等。