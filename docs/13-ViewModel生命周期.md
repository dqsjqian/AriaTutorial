# ViewModel 生命周期：激活、父子树、销毁钩子

![第 13 章配图：激活是幂等的，父子树会传播，销毁钩子按后进先出执行。](../images/ch13-viewmodel.zh.png)

*激活是幂等的，父子树会传播，销毁钩子按后进先出执行。*

前面 12 章都在讲"值怎么流动"。这一章讲**容器**：装这些值的 ViewModel 自己有没有生命周期。

有的，而且很轻 —— 没有事件循环、没有框架基类的重活，只有三个概念：**激活、父子、销毁** 🧬

---

## ⚠️ 一个必须记住的位置

`ViewModel` 在 **`aria::binding`** 命名空间里，不是 `aria::`：

```cpp
using namespace aria;
using namespace aria::binding;   // ViewModel 在这里
```

这和 `Property` / `Computed`（在 `aria::`）不同 —— 因为 ViewModel 属于"绑定层"的概念，而不是响应式核心。

---

## 📄 完整程序

```cpp
// ch13: ViewModel 生命周期 -- 激活、父子树、销毁钩子
//
// ViewModel 在 aria::binding 命名空间, 不是 aria:: 下的。
#include "aria/aria.hpp"
#include "aria/binding/view_model.hpp"

#include <iostream>
#include <memory>

using namespace aria;
using namespace aria::binding;

class CounterVm : public ViewModel {
public:
    Property<int> count{0};
    int           activate_count   = 0;
    int           deactivate_count = 0;
    const char*   label            = "";

    explicit CounterVm(const char* name) : label{name} {}

protected:
    void on_activate() override {
        ++activate_count;
        std::cout << "   [" << label << "] on_activate  (第 " << activate_count << " 次)\n";
    }
    void on_deactivate() override {
        ++deactivate_count;
        std::cout << "   [" << label << "] on_deactivate (第 " << deactivate_count << " 次)\n";
    }
};

int main() {
    std::cout << "== 1. activate / deactivate 是幂等的 ==\n";
    auto vm = std::make_shared<CounterVm>("counter");
    std::cout << "   初始 is_active = " << (vm->is_active().get() ? "true" : "false") << '\n';

    vm->activate();
    vm->activate();
    vm->activate();
    std::cout << "   连续 activate 三次, on_activate 实际执行 "
              << vm->activate_count << " 次\n";

    std::cout << "\n== 2. 子 VM 随父 VM 一起激活 ==\n";
    auto parent = std::make_shared<CounterVm>("parent");
    auto child  = std::make_shared<CounterVm>("child");
    parent->add_child(child);
    parent->activate();
    std::cout << "   父激活次数 = " << parent->activate_count
              << ", 子激活次数 = " << child->activate_count << '\n';

    std::cout << "\n== 3. track: 把订阅挂在 VM 上, VM 销毁时自动断开 ==\n";
    Property<int> source{0};
    auto tracked = std::make_shared<CounterVm>("tracked");
    tracked->track(source.on_changed([](const int& v) {
        std::cout << "   tracked 收到 " << v << '\n';
    }));

    std::cout << "   source = 1\n";
    source = 1;

    std::cout << "   tracked VM 销毁 ...\n";
    tracked.reset();

    std::cout << "   source = 2\n";
    source = 2;
    std::cout << "   (销毁后不再有输出)\n";

    std::cout << "\n== 4. destroy hook 按后进先出执行 ==\n";
    {
        auto hooked = std::make_shared<CounterVm>("hooked");
        hooked->add_destroy_hook([] { std::cout << "   hook 1 执行\n"; });
        hooked->add_destroy_hook([] { std::cout << "   hook 2 执行\n"; });
        hooked->add_destroy_hook([] { std::cout << "   hook 3 执行\n"; });
        std::cout << "   注册了 3 个 hook, 现在销毁 VM:\n";
    }

    return 0;
}
```

**真实运行结果**：

```text
== 1. activate / deactivate 是幂等的 ==
   初始 is_active = false
   [counter] on_activate  (第 1 次)
   连续 activate 三次, on_activate 实际执行 1 次

== 2. 子 VM 随父 VM 一起激活 ==
   [parent] on_activate  (第 1 次)
   [child] on_activate  (第 1 次)
   父激活次数 = 1, 子激活次数 = 1

== 3. track: 把订阅挂在 VM 上, VM 销毁时自动断开 ==
   source = 1
   tracked 收到 1
   tracked VM 销毁 ...
   source = 2
   (销毁后不再有输出)

== 4. destroy hook 按后进先出执行 ==
   注册了 3 个 hook, 现在销毁 VM:
   hook 3 执行
   hook 2 执行
   hook 1 执行
```

---

## 1️⃣ activate / deactivate 是幂等的

```cpp
vm->activate();
vm->activate();
vm->activate();
```

输出：`连续 activate 三次, on_activate 实际执行 1 次`。

**幂等**在真实项目里很重要。界面代码经常会在多个时机触发"进入页面"：路由进入、窗口获得焦点、父容器重新布局……如果 `on_activate` 每次都跑，就会出现重复注册订阅、重复发起请求、计数器翻倍这类问题。

Aria 的选择是：**状态由框架管，只在实际发生状态跃迁时通知你** 🔒

`is_active()` 返回的是一个 `Property<bool>`，所以"这个 VM 是否活跃"本身就是个响应式状态 —— 界面可以绑它。

---

## 2️⃣ 父子树：一次激活整棵子树

```cpp
parent->add_child(child);
parent->activate();
```

输出：

```text
   [parent] on_activate  (第 1 次)
   [child] on_activate  (第 1 次)
   父激活次数 = 1, 子激活次数 = 1
```

父 VM 激活时，子 VM 自动跟着激活。这个机制对应真实的界面层级：一个"设置页"下面挂着"账号页""通知页""隐私页"，切进设置页时整棵子树都该活跃起来。

父 VM 会持有子 VM 的 `shared_ptr`，所以**子 VM 的存活不需要你单独管** —— 父没了，子也就没人引用了。

---

## 3️⃣ track：把所有订阅挂在 VM 上

第 6 章讲 `SubscriptionBag` 时提过"存进成员"。`ViewModel` 直接把这个能力内置了：

```cpp
tracked->track(source.on_changed([](const int& v) {
    std::cout << "   tracked 收到 " << v << '\n';
}));
```

看输出：

```text
   source = 1
   tracked 收到 1
   tracked VM 销毁 ...
   source = 2
   (销毁后不再有输出)
```

`tracked.reset()` 之后，`source = 2` 不再产生任何输出 —— **订阅随 VM 一起消失了**。

意义在于：**你不需要记住"这个 VM 里注册了几条订阅、分别要什么时候断"**。所有通过 `track()` 登记的订阅，在 VM 析构时统一解除。

真实用法通常是"构造时登记，从不手动清理"：

```cpp
class HomeVm : public ViewModel {
public:
    HomeVm(SomeService& svc) {
        track(svc.on_data_update([this](const Data& d) { /* ... */ }));
        track(config.on_changed([this](const Config& c) { /* ... */ }));
    }
};
```

---

## 4️⃣ destroy hook：后进先出

```cpp
hooked->add_destroy_hook([] { std::cout << "   hook 1 执行\n"; });
hooked->add_destroy_hook([] { std::cout << "   hook 2 执行\n"; });
hooked->add_destroy_hook([] { std::cout << "   hook 3 执行\n"; });
```

输出：

```text
   hook 3 执行
   hook 2 执行
   hook 1 执行
```

**后进先出（LIFO）** 🌀 和 `SubscriptionBag` 的反序释放一致，也符合析构直觉：后注册的往往依赖先注册的东西，所以必须先拆。

hook 的执行时机**早于成员析构** —— 这意味着你在 hook 里仍然可以安全访问 `this` 的成员变量。典型用途是"把状态存档"：

```cpp
class EditorVm : public ViewModel {
public:
    EditorVm() {
        add_destroy_hook([this] { persist_draft(draft.get()); });
    }
    Property<std::string> draft;
};
```

---

## 🧭 三个概念的分工

| 概念 | 回答什么问题 |
|---|---|
| `activate()` / `deactivate()` | 这个 VM 现在"活着"吗？（影响数据加载、轮询、定时器） |
| `add_child()` | 谁跟着谁一起活？ |
| `track()` / `add_destroy_hook()` | 这个 VM 死的时候，要收掉什么？ |

---

## 📌 小结

- 📍 `ViewModel` 在 **`aria::binding`** 命名空间，需要单独 include；
- 🔒 `activate()` 幂等，`on_activate()` 只在真实状态跃迁时执行一次；
- 🧬 `add_child()` 建立父子树，父激活会带动整棵子树；
- 🪢 `track(subscription)` 把订阅挂到 VM 上，**VM 析构时统一解除**；
- 🌀 `add_destroy_hook()` 按后进先出执行，且早于成员析构，可以安全访问 `this`。

---

**上一章** 👈 [第 12 章 Validator：校验也是响应式的](12-Validator表单校验.md)
**下一章** 👉 [第 14 章 BindingEngine：把 Property 接到界面](14-BindingEngine.md)

> 📂 本章代码来自本仓库 `demos/ch13_viewmodel/main.cpp`，输出为该程序在 Windows / MSVC 19.51 下的真实打印结果。
