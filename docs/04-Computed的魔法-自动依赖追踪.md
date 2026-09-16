# Computed 的魔法：自动依赖追踪是怎么做到的

`Computed` 是 Aria 里最容易被低估的东西。

它看起来只是"一个会自己更新的派生值"，但真正的价值在于 —— **你永远不用手写依赖列表** 🪄

写过 React 的人都知道 `useMemo(fn, [deps])` 里的 `deps` 有多容易漏；写过 Vue 的人知道 `watch` 的依赖数组错一个就静默失效。Aria 把这个环节整个删掉了。

---

## 📄 完整程序

```cpp
// ch04: Computed 自动依赖追踪 -- 依赖不用手写, 用到了才记上
#include "aria/aria.hpp"

#include <iostream>

using namespace aria;

int main() {
    std::cout << "== 1. 动态依赖: 依赖集随分支变化 ==\n";
    Property<int>  a{1};
    Property<int>  b{2};
    Property<bool> use_b{false};

    Computed<int> value{[&] {
        int x = a.get();
        if (use_b.get()) {
            x += b.get();
        }
        return x;
    }};

    std::cout << "   use_b=false: value = " << value.get()
              << ", 依赖个数 = " << value.dependency_count() << '\n';

    b = 20;
    std::cout << "   改 b 后 value = " << value.get() << " (b 还没被读到, 不算依赖)\n";

    use_b = true;
    std::cout << "   use_b=true : value = " << value.get()
              << ", 依赖个数 = " << value.dependency_count() << '\n';

    b = 30;
    std::cout << "   改 b 后 value = " << value.get() << " (现在传播了)\n";

    std::cout << "\n== 2. 重算时机: 依赖一变就算, 结果会缓存 ==\n";
    int recomputes = 0;
    Property<int>  src{1};
    Computed<int>  doubled{[&] {
        ++recomputes;
        return src.get() * 2;
    }};

    std::cout << "   构造后重算次数 = " << recomputes << " (构造时会求值一次)\n";

    src = 5;
    std::cout << "   改 src 后重算次数 = " << recomputes << " (依赖失效, 立即重算)\n";

    std::cout << "   第一次读 = " << doubled.get()
              << ", 重算次数 = " << recomputes << '\n';
    std::cout << "   第二次读 = " << doubled.get()
              << ", 重算次数 = " << recomputes << " (命中缓存, 不重复算)\n";

    src = 6;
    std::cout << "   再改 src 后重算次数 = " << recomputes << '\n';
    std::cout << "   读一下 = " << doubled.get()
              << ", 重算次数 = " << recomputes << '\n';

    std::cout << "\n== 3. 依赖变化时, 下游自动收到通知 ==\n";
    Property<int>  price{100};
    Computed<int>  with_tax{[&] { return price.get() * 105 / 100; }};

    auto sub = with_tax.bind([](int v) {
        std::cout << "   含税价 -> " << v << '\n';
    });

    std::cout << "   price = 200\n";
    price = 200;

    std::cout << "   price = 200 (写相同的值)\n";
    price = 200;

    return 0;
}
```

**真实运行结果**：

```text
== 1. 动态依赖: 依赖集随分支变化 ==
   use_b=false: value = 1, 依赖个数 = 2
   改 b 后 value = 1 (b 还没被读到, 不算依赖)
   use_b=true : value = 21, 依赖个数 = 3
   改 b 后 value = 31 (现在传播了)

== 2. 重算时机: 依赖一变就算, 结果会缓存 ==
   构造后重算次数 = 1 (构造时会求值一次)
   改 src 后重算次数 = 2 (依赖失效, 立即重算)
   第一次读 = 10, 重算次数 = 2
   第二次读 = 10, 重算次数 = 2 (命中缓存, 不重复算)
   再改 src 后重算次数 = 3
   读一下 = 12, 重算次数 = 3

== 3. 依赖变化时, 下游自动收到通知 ==
   含税价 -> 105
   price = 200
   含税价 -> 210
   price = 200 (写相同的值)
```

---

## 实验一：依赖是"读到了才记" 🎯

这段代码有个分支：

```cpp
Computed<int> value{[&] {
    int x = a.get();
    if (use_b.get()) {      // 分支开关
        x += b.get();       // 只有走这条分支才会读 b
    }
    return x;
}};
```

看输出：

```text
   use_b=false: value = 1, 依赖个数 = 2
   改 b 后 value = 1 (b 还没被读到, 不算依赖)
   use_b=true : value = 21, 依赖个数 = 3
   改 b 后 value = 31 (现在传播了)
```

一步步拆：

| 阶段 | 读到了谁 | 依赖个数 | 改 b 会怎样 |
|---|---|---|---|
| `use_b=false` | `a`、`use_b` | 2 | 不影响（b 不是依赖） |
| 改 `b = 20` | — | 2 | `value` 仍是 1 |
| `use_b=true` | `a`、`b`、`use_b` | **3** | — |
| 改 `b = 30` | — | 3 | `value` 变成 31 ✅ |

这就是**动态依赖**：依赖集不是声明出来的，是每次求值时**实际读了谁**决定的。`use_b` 翻成 `true` 之后，`b` 才成为依赖。

`dependency_count()` 让你能直接观察这件事 —— 从 2 变成 3，肉眼可见 👀

> 💡 这个特性带来的直接好处：条件分支里的昂贵计算，只在真正需要时才建立依赖。没走到的分支，改它不会触发任何重算。

---

## 实验二：重算时机与结果缓存 ⚡

这里有个需要实测才能确认的细节 —— 我原本以为 `Computed` 是"没人读就不算"的惰性求值，但实际跑出来是这样：

```text
   构造后重算次数 = 1 (构造时会求值一次)
   改 src 后重算次数 = 2 (依赖失效, 立即重算)
   第一次读 = 10, 重算次数 = 2
   第二次读 = 10, 重算次数 = 2 (命中缓存, 不重复算)
```

三个结论：

1. **构造时就求值一次** —— `Computed` 不是"用到才算"，创建即算出初值；
2. **依赖一变就重算** —— `src = 5` 这一行之后，重算次数已经从 1 变成 2，无需你去读它；
3. **读多少次都只算一次** —— 连续两次 `get()`，重算次数没有增加。

这意味着 `get()` **永远不会返回过期值**，你也不用担心"读一次触发一次计算"的性能问题。

> ⚠️ 反过来要注意：如果你在 `Computed` 的求值函数里塞了很重的计算，那么**每次依赖变化都会付出这个代价**，即使当时没人在看。重计算适合放到显式调用里，或者用 `Effect` 控制触发时机。

---

## 实验三：下游自动收到通知 📣

```cpp
Property<int>  price{100};
Computed<int>  with_tax{[&] { return price.get() * 105 / 100; }};

auto sub = with_tax.bind([](int v) {
    std::cout << "   含税价 -> " << v << '\n';
});
```

看输出：

```text
   含税价 -> 105
   price = 200
   含税价 -> 210
   price = 200 (写相同的值)
```

两件事同时发生了：

- ✅ `bind` 注册时立刻同步了一次（`含税价 -> 105`，即 `100 * 105 / 100`）；
- ✅ `price = 200` 之后自动推送了新值 210；
- 🔇 最后一行 `price = 200` 是**写相同的值**，等值门拦下，没有任何输出。

注意这一整套流程里，**你从未手动调用过任何"更新"函数**。

---

## 🧠 那它到底是怎么做到的

原理不复杂，但很精巧 —— 一句话概括：

> **求值期间，谁被 `get()` 读了，谁就成为依赖。**

具体走三步：

1. **读取时登记**：`Computed` 求值时打开一个"追踪窗口"，`Property::get()` 发现自己正被追踪，就把自己登记到当前求值者的依赖表里；
2. **写入时失效**：`Property::set()` 遍历自己的下游列表，把依赖它的 `Computed` 标记为失效并重算；
3. **重算时重建依赖**：下次求值会**清空旧依赖表重新收集** —— 这正是实验一里"依赖个数从 2 变成 3"的原因。

所以依赖是**求值的副产品**，不需要你声明。而 `peek()` 就是"读取但跳过第 1 步"—— 这就是第 3 章里 `frozen` 永远返回 14 的原因。

---

## 🕳️ 两个真实存在的坑

### 依赖集会在重算时被重建

因为依赖表每次求值都重建，所以**"这次没走到的分支"会立刻失去依赖关系**。这不是 bug，是设计 —— 但它意味着：如果你写了一堆条件分支，依赖集会随数据变化而变化，`dependency_count()` 是观察它的唯一窗口。

### 循环依赖会被熔断

如果依赖图形成环（A 依赖 B、B 又依赖 A），框架不会无限递归，而是抛 `CircularDependencyError`。这是第 7 章的主题，那里会给出一个可运行的最小复现。

---

## 📌 小结

- 🎯 **依赖自动发现**：求值时读了谁，谁就是依赖。不用手写、也写不错；
- 🔄 **动态依赖**：依赖集会随分支变化，`dependency_count()` 可以观察；
- ⚡ **重算时机**：构造时求值一次，依赖一变立即重算，结果缓存，读多次只算一次；
- 📣 **下游通知**：依赖变化会自动推送给订阅者，配合等值门避免无谓刷新；
- 👁️ **想不建立依赖**：用 `peek()`，这是第 3 章讲过的对照面。

---

**上一章** 👈 [第 3 章 Property 精讲：读写、追踪与等值门](03-Property精讲-读写追踪与等值门.md)
**下一章** 👉 第 5 章 batch / untracked / Effect：精确控制通知范围（编写中）

> 📂 本章代码来自本仓库 `demos/ch04_computed/main.cpp`，输出为该程序在 Windows / MSVC 19.51 下的真实打印结果。
