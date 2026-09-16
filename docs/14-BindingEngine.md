# BindingEngine：把 Property 接到界面

这是全书的转折点 🔄 前面 13 章的所有东西 —— `Property`、`Computed`、`Command`、`ObservableList`、`Validator` —— 都还只是"数据"。从这一章起，数据开始**接到界面上**。

接线只有一个类：`BindingEngine`。它认识 `IViewAdapter`，于是换平台只需要换适配器。

---

## 🧪 本章用了一个"看不见的界面"

真实控件需要 Qt6 / UIKit / JNI，命令行里跑不了。所以本章用一个**内存适配器**代替：

```cpp
#include "common/memory_adapter.hpp"   // 教程用的内存适配器
```

它把每个"控件"实现成一个普通结构体：

```cpp
struct MemoryView : public aria::binding::IView {
    std::string text;      // 相当于 QLabel 的文字
    bool        flag   = false;
    int         integer = 0;
    double      number  = 0.0;
    bool        visible = true;
    bool        enabled = true;
    // ...
};
```

它**不画任何东西**，但把绑定的每一条规则都真实执行了。所以本章所有输出都是真的绑定行为，只是"控件"变成了结构体字段。第 16 章会逐行讲这个适配器怎么实现。

---

## 📄 完整程序

```cpp
// ch14: BindingEngine -- 把 Property 接到界面
//
// 本章用一个"内存适配器"代替真实控件 (见 common/memory_adapter.hpp),
// 这样在命令行里就能看清绑定的每一条规则。
//
// 注意: 所有 bind_* 都返回 void。订阅由 BindingEngine 自己持有,
//       engine 活着绑定就有效, engine 析构时统一解绑。
#include "aria/aria.hpp"
#include "aria/binding/binding_engine.hpp"
#include "aria/runtime/dispatcher.hpp"

#include "common/memory_adapter.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <string>

using namespace aria;
using namespace aria::binding;
using tutorial::MemoryAdapter;
using tutorial::MemoryView;

static const char* yes_no(bool v) { return v ? "true" : "false"; }

int main() {
    auto adapter = std::make_shared<MemoryAdapter>();
    BindingEngine engine{adapter};

    std::cout << "平台: " << adapter->platform_name() << '\n';

    std::cout << "\n== 1. bind_text: 双向文本 ==\n";
    Property<std::string> name{"Aria"};
    MemoryView name_view;

    engine.bind_text(name, name_view);

    std::cout << "   绑定后 view.text = \"" << name_view.text << "\"\n";
    name = "Hello";
    std::cout << "   VM 改值   -> view.text = \"" << name_view.text << "\"\n";
    name_view.user_type("World");
    std::cout << "   用户输入  -> VM = \"" << name.get() << "\"\n";

    std::cout << "\n== 2. bind_int / bind_double: 数值双向 ==\n";
    Property<int>    count{3};
    Property<double> ratio{0.5};
    MemoryView count_view;
    MemoryView ratio_view;

    engine.bind_int(count, count_view);
    engine.bind_double(ratio, ratio_view);

    std::cout << "   count = " << count.get()
              << " -> view.integer = " << count_view.integer << '\n';
    std::cout << "   ratio = " << ratio.get()
              << " -> view.number  = " << ratio_view.number << '\n';

    count = 7;
    std::cout << "   count = 7 -> view.integer = " << count_view.integer << '\n';

    std::cout << "\n== 3. bind_text_projected: 单向投影 ==\n";
    Property<int> amount{100};
    MemoryView label_view;

    engine.bind_text_projected(amount, label_view, [](int v) {
        return std::string("¥") + std::to_string(v) + ".00";
    });

    std::cout << "   label = \"" << label_view.text << "\"\n";
    amount = 250;
    std::cout << "   amount = 250 -> label = \"" << label_view.text << "\"\n";

    std::cout << "\n== 4. bind_optional_text: 空值也要显示 ==\n";
    Property<std::optional<std::string>> nickname{std::nullopt};
    MemoryView nick_view;

    engine.bind_optional_text(
        nickname, nick_view,
        [](const std::string& s) { return std::string("你好, ") + s; },
        std::string("(还没设置昵称)"));

    std::cout << "   空值 -> \"" << nick_view.text << "\"\n";
    nickname = std::string("谦哥");
    std::cout << "   有值 -> \"" << nick_view.text << "\"\n";

    std::cout << "\n== 5. bind_visible / bind_enabled ==\n";
    Property<bool> loading{true};
    MemoryView panel_view;

    engine.bind_visible(loading, panel_view);

    std::cout << "   loading = true  -> panel.visible = " << yes_no(panel_view.visible) << '\n';
    loading = false;
    std::cout << "   loading = false -> panel.visible = " << yes_no(panel_view.visible) << '\n';

    Property<bool> can_submit{false};
    MemoryView button_view;

    engine.bind_enabled(can_submit, button_view);

    std::cout << "   can_submit = false -> button.enabled = " << yes_no(button_view.enabled) << '\n';
    can_submit = true;
    std::cout << "   can_submit = true  -> button.enabled = " << yes_no(button_view.enabled) << '\n';

    std::cout << "\n== 6. bind_command: 按钮点击 ==\n";
    int clicks = 0;
    Command<> refresh([&] { ++clicks; });
    MemoryView refresh_button;

    engine.bind_command(refresh, refresh_button);

    refresh_button.user_click();
    std::cout << "   点一次   -> clicks = " << clicks << '\n';
    refresh_button.user_click();
    std::cout << "   再点一次 -> clicks = " << clicks << '\n';

    std::cout << "\n== 7. 三种派发策略 ==\n";
    auto dispatcher = std::make_shared<runtime::SimpleDispatcher>();

    auto direct_adapter = std::make_shared<MemoryAdapter>();
    BindingEngine direct_engine{direct_adapter, dispatcher,
                                BindingEngine::DispatchPolicy::Direct};

    auto posted_adapter = std::make_shared<MemoryAdapter>();
    BindingEngine posted_engine{posted_adapter, dispatcher,
                                BindingEngine::DispatchPolicy::AlwaysPost};

    Property<int> value{1};
    MemoryView direct_view;
    MemoryView posted_view;

    direct_engine.bind_int(value, direct_view);
    posted_engine.bind_int(value, posted_view);

    std::cout << "   绑定后       : direct = " << direct_view.integer
              << ", posted = " << posted_view.integer << '\n';

    value = 42;
    std::cout << "   改值后立刻   : direct = " << direct_view.integer
              << ", posted = " << posted_view.integer
              << "   (AlwaysPost 还在队列里)\n";

    const std::size_t processed = dispatcher->pump();
    std::cout << "   pump " << processed << " 条后: posted = " << posted_view.integer << '\n';

    return 0;
}
```

**真实运行结果**：

```text
平台: Memory

== 1. bind_text: 双向文本 ==
   绑定后 view.text = "Aria"
   VM 改值   -> view.text = "Hello"
   用户输入  -> VM = "World"

== 2. bind_int / bind_double: 数值双向 ==
   count = 3 -> view.integer = 3
   ratio = 0.5 -> view.number  = 0.5
   count = 7 -> view.integer = 7

== 3. bind_text_projected: 单向投影 ==
   label = "¥100.00"
   amount = 250 -> label = "¥250.00"

== 4. bind_optional_text: 空值也要显示 ==
   空值 -> "(还没设置昵称)"
   有值 -> "你好, 谦哥"

== 5. bind_visible / bind_enabled ==
   loading = true  -> panel.visible = true
   loading = false -> panel.visible = false
   can_submit = false -> button.enabled = false
   can_submit = true  -> button.enabled = true

== 6. bind_command: 按钮点击 ==
   点一次   -> clicks = 1
   再点一次 -> clicks = 2

== 7. 三种派发策略 ==
   绑定后       : direct = 1, posted = 1
   改值后立刻   : direct = 42, posted = 1   (AlwaysPost 还在队列里)
   pump 1 条后: posted = 42
```

---

## ⚠️ 第一个必须说清的点：`bind_*` 返回 `void`

```cpp
engine.bind_text(name, name_view);   // 没有返回值
```

对比第 2 章的 `label.bind(fn)` —— 那个返回 `Subscription`，必须接住。**这里的 `bind_*` 不返回任何东西。**

原因：订阅的所有权在 `BindingEngine` 手里。engine 活着，绑定就有效；engine 析构，所有绑定统一解除。

```cpp
{
    BindingEngine engine{adapter};
    engine.bind_text(name, view);
}   // engine 析构, 绑定结束
```

所以你**不需要**也**不能**写 `auto sub = engine.bind_text(...)` —— 那会编译报错（`void` 不能初始化变量）。

---

## 1️⃣ 双向绑定：改哪边都行

```cpp
engine.bind_text(name, name_view);
```

输出：

```text
   绑定后 view.text = "Aria"        ← 注册时立即同步一次
   VM 改值   -> view.text = "Hello"  ← VM → 界面
   用户输入  -> VM = "World"         ← 界面 → VM
```

三个方向都通了。注意第一行：**绑定完成时就立刻写了一次控件的值** —— 和 `bind` 的初始同步是同一个设计。

### 反馈环是怎么避免的

界面写回 VM 时，会触发 VM 的订阅，理论上又会推回界面 —— 无限循环。Aria 内部用了一个"来源标记"来打断：**由界面写回来的那次变化，不会再推回界面。**

这也是为什么双向绑定能安全用在文本框上。

### 数值类型

`bind_int` / `bind_bool` / `bind_double` / `bind_int64` / `bind_uint64` / `bind_float` 各有对应方法，都是双向：

```cpp
engine.bind_int(count, count_view);      // count = 7 → view.integer = 7
```

---

## 2️⃣ 单向投影：VM 到界面的只读显示

```cpp
engine.bind_text_projected(amount, label_view, [](int v) {
    return std::string("¥") + std::to_string(v) + ".00";
});
```

输出：

```text
   label = "¥100.00"
   amount = 250 -> label = "¥250.00"
```

`bind_text_projected` 的第三个参数是**投影函数**：把模型值转成要显示的文本。它接受 `Property` 和 `Computed` 两种源，所以可以这样组合：

```cpp
Computed<std::string> summary = ...;
engine.bind_text_projected(summary, label, [](const std::string& s) { return s; });
```

⚠️ 但要注意：**单向投影不能用于双向绑定**。`bind_text` 要求源是 `Property`（因为要能写回），传 `Computed` 会编译失败 —— 这是刻意的，只读派生值本来就不该被界面写。

### 空值处理

```cpp
engine.bind_optional_text(nickname, nick_view,
    [](const std::string& s) { return std::string("你好, ") + s; },
    std::string("(还没设置昵称)"));
```

`std::optional<T>` 的源配一个"空值占位文本"，输出：

```text
   空值 -> "(还没设置昵称)"
   有值 -> "你好, 谦哥"
```

不用自己写 `if (has_value())` 分支。

---

## 3️⃣ visible / enabled：状态驱动显示

```cpp
engine.bind_visible(loading, panel_view);      // loading = true → 可见
engine.bind_enabled(can_submit, button_view);  // can_submit = true → 可用
```

这两个是"布尔驱动显示状态"，输出直观：

```text
   loading = true  -> panel.visible = true
   loading = false -> panel.visible = false
```

真实项目里绑定源通常是 `Computed`：

```cpp
Computed<bool> show_error = [&] { return !name.get().empty() && !valid.get(); };
engine.bind_visible(show_error, error_label);
```

于是"错误提示什么时候显示"成了一条纯逻辑规则，跟界面无关。

---

## 4️⃣ bind_command：点击接到动作上

```cpp
engine.bind_command(refresh, refresh_button);
refresh_button.user_click();   // 模拟用户点击
```

输出：

```text
   点一次   -> clicks = 1
   再点一次 -> clicks = 2
```

`bind_command` 除了接点击，还会**写一次控件的 `enabled`**（取自 `Command::can_execute`）。所以在第 20 章你会看到这样一个组合：

```cpp
engine.bind_command(vm.add, add_button);       // 先绑命令
engine.bind_enabled(vm.can_add, add_button);   // 再绑有效状态 —— 后者权威
```

**后绑定的那个决定 `enabled` 的最终值** —— 这是需要注意的顺序问题，Aria 的测试里专门为这个顺序写了用例。

---

## 5️⃣ 三种派发策略：绑定更新走哪条路

```cpp
BindingEngine direct_engine{adapter, dispatcher, DispatchPolicy::Direct};
BindingEngine posted_engine{adapter, dispatcher, DispatchPolicy::AlwaysPost};
```

看输出：

```text
   绑定后       : direct = 1, posted = 1
   改值后立刻   : direct = 42, posted = 1   (AlwaysPost 还在队列里)
   pump 1 条后: posted = 42
```

改值之后、`pump()` 之前，`direct` 已经是 42，而 `posted` 还是 1 —— **`AlwaysPost` 把更新放进了队列**，要等宿主线程 pump 才执行。

| 策略 | 行为 | 什么时候用 |
|---|---|---|
| `Direct` | 立即在调用线程写控件 | 单线程、确定同线程 |
| `SmartMarshal`（默认） | 同线程直接写；跨线程才 post | 大多数场景的默认选择 |
| `AlwaysPost` | 总是进队列，由宿主线程执行 | 强制 UI 线程串行化，最保守 |

`SmartMarshal` 是默认值，因为它兼顾了性能和线程安全：**能直接写就直写，不能才排队**。

---

## 🧭 绑定方法速查

| 方法 | 方向 | 说明 |
|---|---|---|
| `bind_text` / `bind_int` / `bind_bool` / `bind_double` / `bind_int64` / `bind_uint64` / `bind_float` | 双向 | 需要源是 `Property` |
| `bind_*_oneway`（如 `bind_text_oneway`） | 单向 | 源可以是 `Computed` |
| `bind_text_projected` | 单向 | 带投影函数，最常用 |
| `bind_optional_text` | 单向 | `optional` 源 + 空值占位 |
| `bind_text_converted` / `bind_int_converted` | 双向 | 走 `Converter` 做类型往返 |
| `bind_visible` / `bind_enabled` | 单向 | 布尔驱动显示状态 |
| `bind_command` | - | 点击绑定，同时写一次 `enabled` |

---

## 📌 小结

- 🔌 接线三步：造适配器 → 造 `BindingEngine` → `bind_*`；
- 🚫 **`bind_*` 返回 `void`**，订阅归 engine 所有，engine 析构才解绑；
- ↔️ 双向绑定有反馈环抑制，界面写回不会再次推给界面；
- 📊 `bind_text_projected` 是最常用的方法：单向 + 投影函数；
- 🎯 `bind_command` 会顺带写一次 `enabled`，注意与 `bind_enabled` 的**顺序**；
- 🚚 三种派发策略里，默认的 `SmartMarshal` 兼顾性能与线程安全。

---

**上一章** 👈 [第 13 章 ViewModel 生命周期：激活、父子树、销毁钩子](13-ViewModel生命周期.md)
**下一章** 👉 [第 15 章 适配器体系：一份 ViewModel 接五种界面](15-适配器体系.md)

> 📂 本章代码来自本仓库 `demos/ch14_binding/main.cpp`，输出为该程序在 Windows / MSVC 19.51 下的真实打印结果。
