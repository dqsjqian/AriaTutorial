# Putting It Together: A Todo List Built with Aria

The previous nineteen chapters took Aria apart. This one puts the pieces back: **a complete todo list** with an input, a button, a list, and a counter 🔨

The goal is simple: show you what "no UI code in the ViewModel, no business logic on the UI side" looks like in real code.

---

## 📄 The complete program

```cpp
// ch20: 综合实战 -- 用 Aria 写一个待办清单
//
// 本章把前面 19 章的东西串起来:
//   Property / Computed / Command / ObservableList / BindingEngine / 内存适配器
// ViewModel 里没有一行界面代码, 界面侧没有一行业务逻辑。
#include "aria/aria.hpp"
#include "aria/binding/binding_engine.hpp"

#include "common/memory_adapter.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace aria;
using namespace aria::binding;
using tutorial::MemoryAdapter;
using tutorial::MemoryView;

struct Todo {
    Property<std::string> title;
    Property<bool>        done{false};

    explicit Todo(std::string t) : title{std::move(t)} {}
};

// ---------------------------------------------------------------------------
// ViewModel: 纯 C++, 可以脱离任何界面做单元测试
// ---------------------------------------------------------------------------
class TodoViewModel {
public:
    Property<std::string> draft{""};        // 输入框里的草稿
    ObservableList<Todo>  todos;            // 待办列表
    Property<int>         remaining{0};     // 未完成条数

    // 有内容才能添加。这个 Computed 会被绑到按钮的 enabled 上。
    Computed<bool> can_add{[&] { return !draft.get().empty(); }};

    Command<> add{[&] {
        auto item = std::make_shared<Todo>(draft.get());
        todos.push_back(std::move(item));
        draft = "";                          // 添加完清空输入框
        refresh_remaining();
    }};

    TodoViewModel() {
        // 列表本身不属于响应式图 (它是 ObservableList 而不是 Property),
        // 所以统计值要在事件回调里手动刷新。
        todos_sub_ = todos.observe([&](const ListChange<Todo>&) {
            refresh_remaining();
        });
    }

    void toggle(std::size_t index) {
        if (index >= todos.size()) {
            return;
        }
        auto item = todos.at(index);
        item->done = !item->done.get();
        refresh_remaining();
    }

    [[nodiscard]] int done_count() const {
        int n = 0;
        for (const auto& item : todos.snapshot()) {
            if (item->done.get()) {
                ++n;
            }
        }
        return n;
    }

private:
    void refresh_remaining() {
        int n = 0;
        for (const auto& item : todos.snapshot()) {
            if (!item->done.get()) {
                ++n;
            }
        }
        remaining = n;
    }

    Subscription todos_sub_;
};

int main() {
    auto adapter = std::make_shared<MemoryAdapter>();
    BindingEngine engine{adapter};

    TodoViewModel vm;

    // ---- 界面侧: 三个控件, 十几行接线 ----
    MemoryView input_view;
    MemoryView add_button;
    MemoryView stats_label;

    engine.bind_text(vm.draft, input_view);
    engine.bind_command(vm.add, add_button);            // 先绑命令 (会写一次 enabled)
    engine.bind_enabled(vm.can_add, add_button);        // 再绑有效状态, 后者权威
    engine.bind_text_projected(vm.remaining, stats_label, [](int n) {
        return "还剩 " + std::to_string(n) + " 项";
    });

    std::cout << "初始状态\n";
    std::cout << "   输入框     = \"" << input_view.text << "\"\n";
    std::cout << "   添加可点击 = " << (add_button.enabled ? "true" : "false") << '\n';
    std::cout << "   统计       = " << stats_label.text << '\n';

    std::cout << "\n-- 用户输入 \"写教程\" --\n";
    input_view.user_type("写教程");
    std::cout << "   VM.draft   = \"" << vm.draft.get() << "\"\n";
    std::cout << "   添加可点击 = " << (add_button.enabled ? "true" : "false") << '\n';

    std::cout << "\n-- 点击添加 --\n";
    add_button.user_click();
    std::cout << "   列表大小   = " << vm.todos.size() << '\n';
    std::cout << "   输入框清空 = \"" << input_view.text << "\"\n";
    std::cout << "   统计       = " << stats_label.text << '\n';
    std::cout << "   添加可点击 = " << (add_button.enabled ? "true" : "false") << '\n';

    std::cout << "\n-- 再添加两条 --\n";
    input_view.user_type("跑 demo");
    add_button.user_click();
    input_view.user_type("发 GitHub");
    add_button.user_click();
    std::cout << "   列表大小   = " << vm.todos.size() << '\n';
    std::cout << "   统计       = " << stats_label.text << '\n';

    std::cout << "\n-- 勾掉第一项 --\n";
    vm.toggle(0);
    std::cout << "   第一项完成 = " << (vm.todos.at(0)->done.get() ? "true" : "false") << '\n';
    std::cout << "   已完成     = " << vm.done_count() << " 项\n";
    std::cout << "   统计       = " << stats_label.text << '\n';

    std::cout << "\n-- 全部勾完 --\n";
    vm.toggle(1);
    vm.toggle(2);
    std::cout << "   统计       = " << stats_label.text << '\n';

    std::cout << "\n同一个 ViewModel 换成 Qt / UIKit / JNI / HTTP 适配器,\n";
    std::cout << "业务逻辑一行都不用改。\n";

    return 0;
}
```

**Actual output**:

```text
初始状态
   输入框     = ""
   添加可点击 = false
   统计       = 还剩 0 项

-- 用户输入 "写教程" --
   VM.draft   = "写教程"
   添加可点击 = true

-- 点击添加 --
   列表大小   = 1
   输入框清空 = ""
   统计       = 还剩 1 项
   添加可点击 = false

-- 再添加两条 --
   列表大小   = 3
   统计       = 还剩 3 项

-- 勾掉第一项 --
   第一项完成 = true
   已完成     = 1 项
   统计       = 还剩 2 项

-- 全部勾完 --
   统计       = 还剩 0 项

同一个 ViewModel 换成 Qt / UIKit / JNI / HTTP 适配器,
业务逻辑一行都不用改。
```

---

## 🧩 Taking the ViewModel apart

### Three states, one derivation, one action

```cpp
Property<std::string> draft{""};        // input draft
ObservableList<Todo>  todos;            // the list
Property<int>         remaining{0};     // outstanding count

Computed<bool> can_add{[&] { return !draft.get().empty(); }};   // derivation

Command<> add{[&] { ... }};             // the action
```

Those five lines *are* the application's business model. Note that **not one mentions a UI** — no "text field", no "button", no "label". Only draft, list, count, whether adding is allowed, and adding itself.

### The chain reaction inside the action

```cpp
Command<> add{[&] {
    auto item = std::make_shared<Todo>(draft.get());
    todos.push_back(std::move(item));
    draft = "";                          // clear the input
    refresh_remaining();
}};
```

A single `draft = ""` accomplishes "clear the input". From the output:

```text
-- 点击添加 --
   列表大小   = 1
   输入框清空 = ""        ← this line needed no extra code
   统计       = 还剩 1 项
   添加可点击 = false     ← the result of can_add recomputing
```

**One action triggered four knock-on effects**: list length, input contents, the counter, and button availability. Yet `add` only expresses "turn the draft into a todo and clear the draft".

---

## 🔍 An honest detail: why `remaining` is refreshed by hand

This is the most instructive part of the chapter:

```cpp
TodoViewModel() {
    // ObservableList is not a Property, so it is not part of the reactive graph;
    // derived statistics must be refreshed in an event callback.
    todos_sub_ = todos.observe([&](const ListChange<Todo>&) {
        refresh_remaining();
    });
}
```

**`ObservableList` is not a `Property`** — it does not live in the reactive dependency graph. So:

- You cannot write `Computed<int> remaining{[&] { return count_undone(todos); }}` and expect it to recompute — reading a list registers no dependency;
- The correct approach is to **subscribe to list events and update a `Property<int>` in the callback**.

This is a deliberate boundary: *Aria models values as `Property` (in the graph) and collections as `ObservableList` (an event stream)*. They notify through different mechanisms, each suited to its usage pattern.

> 💡 **Practical advice**: when you want a statistic derived from a list (count, sum, is-empty), receive it into a `Property` and update it in an `observe` callback. Do not try to make a `Computed` read the list directly.

Note also `todos_sub_ = todos.observe(...)` — the subscription is stored as a member (Chapter 6's pattern), so it is released when the VM is destroyed.

---

## 🔌 The UI side is a dozen lines

```cpp
engine.bind_text(vm.draft, input_view);
engine.bind_command(vm.add, add_button);            // command first
engine.bind_enabled(vm.can_add, add_button);        // then the state
engine.bind_text_projected(vm.remaining, stats_label, [](int n) {
    return "还剩 " + std::to_string(n) + " 项";
});
```

Four lines, three widgets (input, button, label).

### About the ordering of two of those lines

```cpp
engine.bind_command(vm.add, add_button);            // writes enabled once
engine.bind_enabled(vm.can_add, add_button);        // this one wins
```

As Chapter 14 noted, `bind_command` also writes the widget's `enabled`, and **the later binding decides the final value**. Binding the command first and `can_add` second means the button's availability is governed by `can_add`.

The output confirms the ordering took effect:

```text
初始状态      添加可点击 = false      ← can_add is false (empty draft)
用户输入后    添加可点击 = true       ← can_add became true
点击添加后    添加可点击 = false      ← draft cleared, can_add false again
```

That "auto-disable after adding" behaviour **has no dedicated line of code** — it falls out of `draft = ""`.

---

## 🌍 Swapping in a real platform

This chapter uses `MemoryAdapter` so it can run from a command line. On a real platform, only the opening two lines of `main` change:

**Qt6**

```cpp
auto adapter = std::make_shared<aria::adapters::qt6::QtAdapter>();
BindingEngine engine{adapter};

engine.bind_text(vm.draft, adapter->view_for(line_edit));
engine.bind_command(vm.add, adapter->view_for(add_button));
engine.bind_enabled(vm.can_add, adapter->view_for(add_button));
engine.bind_text_projected(vm.remaining, adapter->view_for(stats_label),
    [](int n) { return QString("还剩 %1 项").arg(n).toStdString(); });
```

**HTTP**

```cpp
auto http = std::make_shared<aria::adapters::http::HttpAdapter>(config);
BindingEngine engine{http, dispatcher, DispatchPolicy::SmartMarshal};

auto& input  = http->register_view("draft",    "text");
auto& button = http->register_view("add",      "button");
auto& stats  = http->register_view("stats",    "text");

engine.bind_text(vm.draft, input);
engine.bind_command(vm.add, button);
engine.bind_enabled(vm.can_add, button);
engine.bind_text_projected(vm.remaining, stats, [](int n) {
    return "还剩 " + std::to_string(n) + " 项";
});
```

**`TodoViewModel` does not change a single character** ✅ That is the thing this series has been building toward since Chapter 1.

---

## 🎓 What these 20 chapters actually used

| Chapter | Used for |
|---|---|
| 3 `Property` | `draft` / `remaining` |
| 4 `Computed` | `can_add` |
| 6 `Subscription` | the `todos_sub_` member |
| 8 `Command` | `add` |
| 9 `ObservableList` | `todos` + `observe` |
| 14 `BindingEngine` | four `bind_*` calls |
| 15 adapters | `MemoryAdapter`, swappable for Qt6 / HTTP |

**What was not used**: `Effect`, derived views, `reconcile`, `Validator`, the `ViewModel` base class, coroutines, diagnostics. Those are tools you reach for when a need appears — not a checklist to complete.

That is itself a lesson worth keeping: **Aria's core API surface is small, and you use as much of it as your problem demands.**

---

## 📌 Summary

- 🧱 A complete application = a few `Property`s + one `ObservableList` + one or two `Computed`s + one or two `Command`s;
- 🔗 One action can trigger several knock-on effects, **with no per-effect code**;
- ⚠️ `ObservableList` is not in the reactive graph; statistics derived from a list update a `Property` from an `observe` callback;
- 🔌 The UI side is a dozen binding lines; the order of `bind_command` and `bind_enabled` decides the final `enabled`;
- 🌍 Switching platforms changes only the adapter construction and `view_for` / `register_view` — **the ViewModel is untouched**.

---

**Previous chapter** 👈 [Chapter 19 — Testing: How to Know Your Reactive Code Is Correct](19-testing.en.md)

**Series complete** 🎉 Across 20 chapters you have covered Aria's reactive core, binding layer, and adapter layer, plus the engineering surrounds: async, diagnostics, and testing.

To go deeper, the `Aria` repository's `docs/` holds the numbered contract documents (`L-N` in `lifecycle.md`, `E-N` in `error-model.md`, `LD-N` in `list-diff-contract.md`, and so on) — every non-trivial behaviour pinned to a traceable clause.

> 📂 Code from `demos/ch20_ecosystem/main.cpp`. Output is the program's real stdout on Windows / MSVC 19.51.
