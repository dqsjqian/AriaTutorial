# 关于本仓库

## 这是什么

**AriaTutorial 是一套 20 章、中英双语的 Aria 教程，每一章都配一个可以编译、可以运行、能看到输出的最小 demo。**

它不是为了"介绍 API 有哪些"，而是为了回答三个更实际的问题：

1. **这个框架解决什么问题**，以及它明确的代价是什么（第 1 章）；
2. **每一条行为到底是怎么发生的**——由可运行的程序印出来，而不是由文档承诺；
3. **真实项目里该怎么用**——从单个 `Property` 到完整应用（第 20 章）。

## 面向谁

| 读者 | 从哪里开始 |
|---|---|
| 听说过 Aria，想知道值不值得用 | 第 1 章（含取舍表）、第 20 章（完整应用） |
| 决定用了，需要上手 | 第 2 章构建，然后第 3-8 章吃透响应式核心 |
| 要接真实界面 | 第 9-14 章（集合/表单/绑定），第 15-16 章（适配器） |
| 要处理异步与工程质量 | 第 17-19 章（协程/诊断/测试） |
| 只想看代码 | 直接读 `demos/`，每章一个 `main.cpp` |

熟悉 Vue / React / Qt 的读者上手会更快：`Property` ≈ `ref`，`Computed` ≈ `computed`/`useMemo`（但不用写依赖数组），`ObservableList` 类似带变更事件的集合。

## 内容如何组织

```
docs/          20 章正文，中英双版本（NN-标题.md / NN-english-title.en.md）
demos/         每章一个可运行目标（ch01_bill … ch20_ecosystem）
demos/common/  各章共用的教程用内存适配器
images/        每章一张配图，中英各一版；SVG 源在 images/src/
```

**正文和 demo 是同源的，但方向是反的**：demo 先写完并真实跑通，正文再从它的源码与 stdout 反向产出。所以正文里的每段代码都能在 `demos/` 里找到出处。

**配图也同样受约束**。每章标题下方那张图不是装饰，而是那一章的结论图：数据流、依赖图、状态机或时间线。图里出现的数字（重算次数、事件数、并发耗时）全部取自该章 demo 的真实输出，与正文里的 ```text 块是同一份数据。图有 SVG 源文件，可以改文案后重新导出。

## 质量标准

本仓库对"真实性"有硬性要求，并且是**自动化校验**的，不依赖人眼：

| 检查项 | 规则 |
|---|---|
| 代码真实性 | 正文里每个 ```cpp 代码块必须与对应 `demos/<章>/main.cpp` **逐字一致** |
| 输出真实性 | 正文里每个 ```text 输出块必须与该 demo 的**真实 stdout 逐字一致** |
| 配图真实性 | 每篇恰好引用一张配图，文件存在，且像素尺寸与它的 SVG 源一致 |
| 编译通过 | 全部 demo 必须编译通过（MSVC / Windows 实测，代码本身跨平台） |

任何一项不通过都会直接报错。也就是说：**你在正文里读到的每一条行为描述，都可以靠 `build/bin/chNN_xxx` 亲手复现。**

正文中出现的全部输出，都是在 Windows / MSVC 19.51 下真实运行打印的，不是示意。

## 明确不覆盖的部分

诚实划界，避免误导：

| 项目 | 说明 |
|---|---|
| **第 15 章的 Qt6 / HTTP demo** | 需要 Qt6 与 Aria HTTP 模块，默认不参与构建，本教程**未运行验证**其输出。正文只校验代码来源 |
| **AppKit / UIKit / JNI 适配器** | 需要对应平台构建。正文给出接线代码与对照表，**不声称已验证** |
| 界面视觉与交互测试 | 属于 UI 测试范畴，本教程不覆盖 |
| Aria 全部 API | 本教程覆盖主线；带编号的契约细节见 Aria 仓库的 `docs/` |

## 如何构建并验证一切

```bash
git clone https://github.com/dqsjqian/AriaTutorial.git
cd AriaTutorial

# Aria 已安装
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 或用源码树
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DARIA_ROOT=../Aria

cmake --build build -j

# 逐个运行，看到与正文一致的输出
./build/bin/ch01_bill
./build/bin/ch20_ecosystem
```

一键脚本：`scripts/run-all.sh`（macOS / Linux）或 `scripts/run-all.ps1`（Windows）。

## 与 Aria 的关系

本仓库是 [Aria](https://github.com/dqsjqian/Aria) 的**教学配套仓库**，由 Aria 维护者编写，不包含框架源码本身。

- 框架本体、契约文档、API 参考 → [Aria](https://github.com/dqsjqian/Aria)
- 旗舰跨端示例 → [AriaTools](https://github.com/dqsjqian/AriaTools)
- 本仓库 → 从零到能用的学习路径

## 贡献

欢迎修正错误、补充示例、改进双语表述。请遵循两条原则：

1. **改动 demo 源码时，同步更新对应正文的代码块与输出块**，否则校验会失败；
2. **不要在正文里写"示意性输出"**——要么真跑，要么不放。

改动配图时，请同时更新 `images/src/*.svg` 与导出的 PNG，并保证图里的数字仍与 demo 输出一致。

涉及新增章节的，建议先开 Issue 说明大纲。

## License

[MIT](LICENSE) © 2026 aria contributors
