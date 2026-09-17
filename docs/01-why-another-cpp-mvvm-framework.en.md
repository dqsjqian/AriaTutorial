# Why Another C++ MVVM Framework

![Chapter 1 figure: Left: sync code grows with the number of platforms. Right: Aria splits out one layer, so a new platform costs one adapter.](../images/ch01-why-aria.en.svg)

*Left: sync code grows with the number of platforms. Right: Aria splits out one layer, so a new platform costs one adapter.*

> The code in this chapter is reproduced verbatim from `demos/ch01_bill/main.cpp`.
> Source comments are in Chinese, matching the repository. The program's output
> is also in Chinese for the same reason.

## 🧾 A scenario you have almost certainly written

Suppose you are building an expense-splitting app. The core logic is "split the bill" — total divided by headcount:

```cpp
double per_person = bill / people;
```

Nothing remarkable about that line 😌. What actually hurts is **the ring of code around it**.

Change the bill and the label must refresh. Change the headcount and it must refresh again. Change both and the UI flashes the intermediate value. And if the newly computed value equals the one already on screen, the whole refresh was pointless. So you write:

```cpp
void onBillChanged()   { updateLabel(); markDirty(); }
void onPeopleChanged() { updateLabel(); markDirty(); }

void updateLabel() {
    auto v = bill / people;
    if (v != last_shown) {        // manual equality check
        last_shown = v;
        label->setText(format(v));
    }
}
```

Then you add a "batch edit" entry point to stop the flashing 🙈, then a re-entrancy guard to stop feedback loops 🌀, then…

Notice anything? **Not one of those lines describes your business.** They all do the same thing: shuffling "a value changed, so sync it somewhere" from one place to another.

---

## 🎯 The real problem

If this app only ever ships on Qt, you write that ring of code once and live with it.

But the same business logic usually has to ship as desktop, mobile, and web. Qt signals, iOS KVO, Android LiveData, React state — that is **four entirely different state-synchronisation mechanisms** 🌪️

So the splitting logic itself is one line, while the code around it exists four times over. Worse, those four copies **drift apart** as requirements evolve.

> 💡 The cost is not writing it. The cost is **maintaining four copies of code that should mean exactly the same thing**.

---

## 🧱 Existing options, and where each one stops

| Option | What it gives you | What it costs |
|---|---|---|
| **Qt + signals** | Mature desktop state sync | Business logic must inherit `QObject`; useless outside Qt |
| **Flutter / Qt Quick** | One codebase, UI included | UI must be rewritten in their DSL, outside the C++ ecosystem |
| **Rx family** | Powerful stream composition | Steep curve, deep stacks, still usually tied to a platform |
| **WPF / MAUI** | Excellent within .NET | .NET only — a C++ core cannot plug in |

See the common thread? **They all sell "reactivity" bundled with one specific UI framework** 🧷

If your core is C++ and you want each platform to keep its native UI, every one of these options asks you to **hand the core over first**.

---

## ✂️ Aria's answer: split exactly one layer

Aria does one thing:

> **It lifts the reactive engine and the binding layer out of the UI framework, into a plain C++ library that depends on no UI toolkit.**

It cuts the app in two:

- 🧠 **ViewModel** — an ordinary C++ class. No framework base class, no macros, no code generator. It has no idea what is driving it;
- 🎨 **View** — the platform's native widgets. Qt still uses Designer, iOS still uses Storyboard, web still uses HTML;
- 🔌 **`BindingEngine`** sits between them and only knows one interface: `IViewAdapter`.

**Switching platforms means switching the adapter — not the business logic.**

---

## 👀 See it in one program

Here is that bill-splitting app. It only depends on `aria::core` and runs from the command line:

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

**Actual output** — not an illustration, this is what the program really prints:

```text
每人应付: 50.00 元
-- 改成 4 个人分摊 --
每人应付: 25.00 元
-- 总额改成 200 --
每人应付: 50.00 元
-- batch: 两处一起改, 只推一次 --
每人应付: 150.00 元
```

Three things happened, and **the framework did all of them** ✨

1. 🪄 **Nobody called `updateLabel()`.** `per_person` discovered its dependencies the first time it evaluated — it read `bill` and `people`, so those became its dependencies. You wrote no "when people changes, update the label" code.
2. 🚫 **The two assignments inside `batch` produced one push, not two.** Without `batch`, the UI would flash `1200/2 = 600` before settling on `150`.
3. 🔇 **An unchanged result pushes nothing.** That is the default — writing an equal value is silently dropped.

`BillViewModel` is a plain struct. It can be unit-tested with no UI involved, and it **does not get rewritten** just because it now has to run on an iPhone.

---

## 🌍 Five platforms, one ViewModel

Look at the wiring on each platform. They are nearly identical, and that `BillViewModel` **did not change a single character**.

**🖥️ Qt6 / Windows / macOS / Linux**

```cpp
auto adapter = std::make_shared<aria::adapters::qt6::QtAdapter>();
aria::binding::BindingEngine engine{adapter};

BillViewModel vm;
aria::adapters::qt6::QtView label_view{real_label};   // wraps the QLabel from Designer
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

**🌐 Web (C++ runs on the server; the browser is just HTML/JS)**

```cpp
aria::adapters::http::HttpAdapterConfig config;
config.port = 9090;
auto http = std::make_shared<aria::adapters::http::HttpAdapter>(config);

auto& total = http->register_view("total", "text");   // a "widget" is a string id here

aria::binding::BindingEngine engine{http};
engine.bind_text_projected(vm.per_person, total,
    [](double v) { return std::format("¥{:.2f}", v); });
http->start();   // Property changes go out over SSE; user input comes back over REST
```

Wiring is always three steps:

> **Create the platform adapter → build a `BindingEngine` with it → bind widgets to Properties.**

One thing must be stated plainly: **you do not write the UI in C++** 🔨 Buttons, layout, and animation still come from Qt Designer, Storyboard, Compose, or HTML. Those dozen lines only hand already-existing widgets to the engine.

---

## ⚠️ Know the cost before you choose it

This section matters more than everything above. Aria makes deliberate trade-offs, and **it is not for everyone**.

| Trade-off | Detail |
|---|---|
| 🧩 **C++20 minimum** | Requires full coroutines and concepts (GCC 12+ / Clang 15+ / MSVC v143). C++17 projects are out. |
| 🎨 **No widgets** | Aria draws nothing. Widgets, layout, and animation stay with your UI toolkit. |
| 🔗 **Templates are source-compatible only** | `aria-abi` / `aria-runtime` / `aria-binding` are ABI-stable within a major version; templates like `Property<T>` need a recompile across versions. |
| 🔧 **You may need to write an adapter** | Five ship out of the box: Qt6 / AppKit / UIKit / JNI / HTTP. A new toolkit means implementing `IViewAdapter`. |
| 🌱 **Young project** | Ecosystem, tutorials, and third-party components cannot match mature frameworks. Today AriaTools is the one real application using it. |

✅ **Good fit**: you already have a C++ core, need that logic on multiple platforms, and want each platform to keep its native UI.

❌ **Bad fit**: you want "one codebase including the UI". That is the territory of full UI frameworks like Flutter and Qt Quick — Aria does not try to replace them.

---

## 📌 Summary

- 💸 The cost of hand-written state sync is not writing it — it is **maintaining several copies that should be identical**;
- 🧷 Every existing option bundles reactivity with one specific UI framework, leaving a C++ core outside;
- ✂️ Aria splits exactly one layer: reactive engine plus binding, as a plain C++ library, with UI attached through `IViewAdapter`;
- 🎁 What you gain: automatic dependency tracking in `Property` / `Computed`, intermediate states suppressed by `batch`, and equal writes dropped — **with zero sync code written by you**;
- ⚖️ What it costs: C++20 minimum, no widgets, young ecosystem. If that is unacceptable, do not pick it.

---

**Next chapter** 👉 [Chapter 2 — First Reactive Program in Ten Minutes](02-first-reactive-program-in-ten-minutes.en.md) — build Aria from scratch and run the code above for real.

> 📂 Code from `demos/ch01_bill/main.cpp`. Output is the program's real stdout on Windows / MSVC 19.51.
