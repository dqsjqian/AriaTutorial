# 为什么再造一个 C++ MVVM 框架

## 🧾 一个你肯定写过的场景

假设你写了个记账 App，核心逻辑是「AA 制分摊」——总额除以人数，算出每人应付：

```cpp
double per_person = bill / people;
```

这一行没啥可讲的 😌。真正麻烦的是**围着它的那一圈代码**。

账单改了要刷新界面，人数改了也要刷新；两个一起改的时候中间态会闪一下；如果新算出来的和上次一模一样，其实根本不用刷新。于是你写下：

```cpp
void onBillChanged()   { updateLabel(); markDirty(); }
void onPeopleChanged() { updateLabel(); markDirty(); }

void updateLabel() {
    auto v = bill / people;
    if (v != last_shown) {        // 手动做等值判断
        last_shown = v;
        label->setText(format(v));
    }
}
```

接着再加个「批量修改」入口避免闪烁 🙈，再加个防重入标志避免回环 🌀，再加个……

发现了吗？这些代码**没有一个字在描述业务**。它们全都在干同一件事：把「值变了要同步到界面」这件事，从一个地方搬到另一个地方。

---

## 🎯 问题的本质

如果这 App 只有 Qt 一端，上面那些代码写一遍也就认了。

但现实是：同一个业务往往要出桌面版、移动版、Web 版。而 Qt 的信号槽、iOS 的 KVO、Android 的 LiveData、Web 的 React state，是**四套完全不同的状态同步机制** 🌪️

于是「分摊逻辑」本体只有 1 行，围着它的同步代码却有 4 份。更要命的是，这 4 份会随着需求演进**各自跑偏**。

> 💡 真正的成本不在写，在**维护 4 份语义本该一致的代码**。

---

## 🧱 现有方案，各自的边界

| 方案 | 能做到什么 | 代价 |
|---|---|---|
| **Qt + 信号槽** | 桌面端状态同步很成熟 | 业务逻辑要继承 `QObject`，离开 Qt 就用不了 |
| **Flutter / Qt Quick** | 一份代码连界面一起跨端 | 界面要用它的 DSL 重写，且脱离 C++ 生态 |
| **Rx 系列** | 流式组合能力强，生态大 | 学习曲线陡，调试栈深，通常仍与某平台绑定 |
| **WPF / MAUI** | .NET 里体验最好 | 只在 .NET，C++ 内核接不进来 |

看出来了吗？它们有个共同点：**都把「响应式」和「某个具体 UI 框架」绑在一起卖** 🧷

如果你的业务内核是 C++，又希望各端保留原生界面，你会发现这些方案都要求你**先把 C++ 内核交出去**。

---

## ✂️ Aria 的答案：只拆一层

Aria 只做一件事：

> **把响应式引擎和绑定层从 UI 框架里拆出来，做成不绑定任何 UI 工具包的纯 C++ 库。**

它把界面切成两半：

- 🧠 **ViewModel** 是普通 C++ 类 —— 不继承框架基类、不需要宏、不需要代码生成器，它不知道自己在被谁驱动；
- 🎨 **View** 是各平台原生控件 —— Qt 的照旧用 Designer 拖，iOS 的还是 Storyboard，Web 的还是 HTML；
- 🔌 中间由 `BindingEngine` 连接，而它只认 `IViewAdapter` 一个接口。

**换平台换的是适配器，不是业务逻辑。**

---

## 👀 一段代码看懂

下面就是那个 AA 制账单。整个程序只依赖 `aria::core`，命令行下就能跑：

```cpp
// ch01: 同一份 ViewModel 驱动五个平台, 而它自己不依赖任何 UI。
// 这里用命令行验证这件事: 只改数据, 派生值自动重算。
#include "aria/aria.hpp"

#include <format>
#include <iostream>

using namespace aria;

// 这一段就是"业务逻辑"。它不知道自己是跑在 Qt、iOS、Android 还是浏览器里。
struct BillViewModel {
    Property<double> bill{100.0};  // 账单总额
    Property<int>    people{2};    // 参与人数

    // 依赖不用手写: 首次求值时读到了 bill 和 people, 就自动记下这两条依赖
    Computed<double> per_person{[this] { return bill.get() / people.get(); }};
};

int main() {
    BillViewModel vm;

    // bind: 先立刻用当前值调一次, 之后每次变化再调一次
    auto sub = vm.per_person.bind([](double v) {
        std::cout << std::format("每人应付: {:.2f} 元\n", v);
    });

    std::cout << "-- 改成 4 个人分摊 --\n";
    vm.people = 4;

    std::cout << "-- 总额改成 200 --\n";
    vm.bill = 200.0;

    std::cout << "-- batch: 两处一起改, 只推一次 --\n";
    aria::reactive::batch([&] {
        vm.bill   = 1200.0;
        vm.people = 8;
    });

    return 0;
}
```

**真实运行结果**（不是示意，是这程序真打印出来的）：

```text
每人应付: 50.00 元
-- 改成 4 个人分摊 --
每人应付: 25.00 元
-- 总额改成 200 --
每人应付: 50.00 元
-- batch: 两处一起改, 只推一次 --
每人应付: 150.00 元
```

注意三件事，它们**都是框架自动完成的** ✨

1. 🪄 **没有人调用过 `updateLabel()`。** `per_person` 的依赖是它首次求值时读到 `bill` 和 `people` 自动记下的 —— 你没写过任何"people 变了要更新"的代码。
2. 🚫 **`batch` 里的两次赋值只产生了一次推送。** 如果不包 `batch`，界面会先闪到 `1200/2=600` 再跳到 `150`。
3. 🔇 **算出来的值没变就不会推送。** 这是框架的默认行为 —— 等值写入静默丢弃。

这份 `BillViewModel` 是个普通 struct，可以脱离任何界面做单元测试。它**不会**因为要跑在 iPhone 上而改写。

---

## 🌍 五个平台，同一份 ViewModel

看一眼各端的接线代码。你会发现它们长得几乎一样，而上面那个 `BillViewModel` **一个字都没改过**。

**🖥️ Qt6 / Windows / macOS / Linux**

```cpp
auto adapter = std::make_shared<aria::adapters::qt6::QtAdapter>();
aria::binding::BindingEngine engine{adapter};

BillViewModel vm;
aria::adapters::qt6::QtView label_view{real_label};   // 包住在 Designer 里拖出的 QLabel
engine.bind_text_projected(vm.per_person, label_view,
    [](double v) { return std::format("¥{:.2f}", v); });
```

**📱 iOS / UIKit**

```objc++
auto adapter = std::make_shared<aria::adapters::uikit::UIKitAdapter>();
aria::binding::BindingEngine engine(adapter, ui_dispatcher,
    aria::binding::BindingEngine::DispatchPolicy::SmartMarshal);

auto label = std::make_shared<aria::adapters::uikit::UIKitView>(self.totalLabel);
engine.bind_text_projected(vm.per_person, *label,
    [](double v) { return std::format("¥{:.2f}", v); });
```

**🌐 Web（C++ 跑在服务端，浏览器只有 HTML/JS）**

```cpp
aria::adapters::http::HttpAdapterConfig config;
config.port = 9090;
auto http = std::make_shared<aria::adapters::http::HttpAdapter>(config);

auto& total = http->register_view("total", "text");   // "控件"在这里是字符串 ID

aria::binding::BindingEngine engine{http};
engine.bind_text_projected(vm.per_person, total,
    [](double v) { return std::format("¥{:.2f}", v); });
http->start();   // Property 变化经 SSE 推给浏览器，用户操作经 REST 回来
```

接线永远是三步：

> **造平台适配器 → 用它造 `BindingEngine` → 把控件和 Property 绑上。**

还有一点必须说清楚：**界面不用 C++ 写** 🔨 按钮、布局、动画照旧用 Qt Designer、Storyboard、Compose、HTML。上面这十几行只是"把已经存在的控件交给引擎"的接线代码。

---

## ⚠️ 选它之前，先看清代价

这一节比前面所有内容都重要。Aria 有明确的取舍，**它不适合所有人**。

| 取舍 | 说明 |
|---|---|
| 🧩 **最低要求 C++20** | 需要完整的协程与 concepts 支持（GCC 12+ / Clang 15+ / MSVC v143）。C++17 项目用不了。 |
| 🎨 **不提供控件** | Aria 不画任何界面。控件、布局、动画仍由你选的 UI 工具包负责。 |
| 🔗 **模板层仅源码兼容** | `aria-abi` / `aria-runtime` / `aria-binding` 在主版本号内 ABI 稳定；`Property<T>` 这类模板跨版本需要重编。 |
| 🔧 **适配器要自己补** | 目前开箱可用的是 Qt6 / AppKit / UIKit / JNI / HTTP 五个。接新工具包意味着实现一个 `IViewAdapter`。 |
| 🌱 **年轻项目** | 生态、教程、第三方组件无法与成熟框架相比。目前只有 AriaTools 一个真实应用在用。 |

✅ **适合**：已有 C++ 业务内核、要在多端复用同一份逻辑、且希望各端保留原生 UI 的项目。

❌ **不适合**：想要「一份代码连界面一起跨端」的场景 —— 那是 Flutter、Qt Quick 这类完整 UI 框架的领域，Aria 不试图取代它们。

---

## 📌 小结

- 💸 手写状态同步的成本不在写，在**维护多份本该一致的代码**；
- 🧷 已有方案都把响应式和某个具体 UI 框架绑在一起，C++ 内核接不进去；
- ✂️ Aria 只拆一层：响应式引擎 + 绑定层做成纯 C++ 库，UI 通过 `IViewAdapter` 接入；
- 🎁 核心收益：`Property` / `Computed` 自动依赖追踪、`batch` 抑制中间态、等值写入不推送 —— **这些都不需要你写一行同步代码**；
- ⚖️ 代价：C++20 起步、不提供控件、生态年轻。接受不了这些就不要选它。

---

**下一章** 👉 [第 2 章 十分钟跑起第一个响应式程序](02-十分钟跑起第一个响应式程序.md)——从零构建 Aria，把上面这段代码真正跑起来。

> 📂 本章代码来自本仓库 `demos/ch01_bill/main.cpp`，输出为该程序在 Windows / MSVC 19.51 下的真实打印结果。
