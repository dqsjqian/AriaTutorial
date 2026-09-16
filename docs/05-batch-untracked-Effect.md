# batch / untracked / Effect：精确控制通知范围

前四章讲的是"怎么让数据自动流动"。这一章讲**怎么在需要的时候踩刹车** 🛑

三个工具各有分工：

| 工具 | 解决什么问题 |
|---|---|
| `batch` | 一批写入只推一次，消除中间态 |
| `untracked` | 读一个值，但不建立依赖 |
| `Effect` | 把"副作用"变成可启动、可停止的对象 |

---

## 📄 完整程序

```cpp
// ch05: batch / untracked / Effect -- 精确控制"什么时候通知"
#include "aria/aria.hpp"

#include <iostream>

using namespace aria;

int main() {
    std::cout << "== 1. 逐个改 vs 包进 batch ==\n";
    Property<int>  bill{100};
    Property<int>  people{2};
    Computed<int>  per_person{[&] { return bill.get() / people.get(); }};

    auto sub = per_person.bind([](int v) {
        std::cout << "   每人 = " << v << '\n';
    });

    std::cout << "   -- 逐个改: 每改一次推一次, 中间态会闪 --\n";
    bill = 300;
    people = 4;

    std::cout << "   -- 包进 batch: 只在结束时推一次 --\n";
    aria::batch([&] {
        bill   = 1200;
        people = 8;
    });

    std::cout << "\n== 2. untracked: 读值但不建立依赖 ==\n";
    Property<int> a{0};
    Property<int> b{0};
    int runs = 0;

    Effect e{[&] {
        const int x = a.get();
        const int y = reactive::untracked([&] { return b.get(); });
        ++runs;
        std::cout << "   effect 第 " << runs << " 次: a = " << x << ", b = " << y << '\n';
    }};

    std::cout << "   b = 99 (untracked 读的, 不触发)\n";
    b = 99;

    std::cout << "   a = 1 (追踪到的, 触发)\n";
    a = 1;

    std::cout << "\n== 3. Effect 的停止 ==\n";
    Property<int> counter{0};
    int ticks = 0;

    Effect ticker{[&] {
        counter.get();
        ++ticks;
    }};

    counter = 1;
    counter = 2;
    std::cout << "   停止前 ticks = " << ticks << '\n';

    ticker.stop();
    counter = 3;
    std::cout << "   停止后 ticks = " << ticks
              << ", active = " << (ticker.active() ? "true" : "false") << '\n';

    return 0;
}
```

**真实运行结果**：

```text
== 1. 逐个改 vs 包进 batch ==
   每人 = 50
   -- 逐个改: 每改一次推一次, 中间态会闪 --
   每人 = 150
   每人 = 75
   -- 包进 batch: 只在结束时推一次 --
   每人 = 150

== 2. untracked: 读值但不建立依赖 ==
   effect 第 1 次: a = 0, b = 0
   b = 99 (untracked 读的, 不触发)
   a = 1 (追踪到的, 触发)
   effect 第 2 次: a = 1, b = 99

== 3. Effect 的停止 ==
   停止前 ticks = 3
   停止后 ticks = 3, active = false
```

---

## 1️⃣ batch：让中间态消失

看第一段输出。**逐个改**的时候推送发生了两次：

```text
   每人 = 150      ← bill 先改成 300, 此时 people 还是 2
   每人 = 75       ← people 再改成 4
```

`300/2 = 150` 这个值**你根本不想要** —— 它只是两次写入之间的必然产物。用户会看到界面闪一下 😵

包进 `batch` 之后：

```cpp
aria::batch([&] {
    bill   = 1200;
    people = 8;
});
```

只推了一次 `150`，而 `1200/8 = 150` 恰好等于之前的值，所以连这一次推送都被等值门吃掉了 —— 输出里 `每人 = 150` 出现的最后一次，是上一步留下的。

> 💡 **经验法则**：凡是"改多个字段才能算出一个有效状态"的场景，都该包 `batch`。表单批量填充、列表批量更新、主题整体切换，全在这一类。

`batch` 可以嵌套，最外层结束时才 flush。

---

## 2️⃣ untracked：读了但不算依赖

```cpp
Effect e{[&] {
    const int x = a.get();                                    // 追踪
    const int y = reactive::untracked([&] { return b.get(); }); // 不追踪
    ...
}};
```

看输出：

```text
   effect 第 1 次: a = 0, b = 0     ← 构造时执行一次
   b = 99 (untracked 读的, 不触发)   ← 没有第三次执行
   a = 1 (追踪到的, 触发)
   effect 第 2 次: a = 1, b = 99    ← a 变了, 重新执行并读到最新的 b
```

`b = 99` 只让 Effect 读到新值，**没有触发重新执行** —— 因为那次读取被包在 `untracked` 里，没有登记依赖。

这个工具解决的是一类隐蔽问题：**你在计算里顺手读了某个值做日志或调试，结果它变成了依赖，导致这个 Effect 被无谓地反复触发。**

> ⚠️ `untracked` 的语义是"只影响追踪，不影响读取"。它返回容器内表达式的值，所以可以直接赋给变量。

---

## 3️⃣ Effect：可启动、可停止的副作用

`Effect` 和 `Computed` 的区别在于**没有返回值**。它不产生新值，只执行副作用 —— 写日志、存盘、调外部接口、驱动非响应式的老控件。

```cpp
Effect ticker{[&] {
    counter.get();
    ++ticks;
}};
```

注意构造时它**立即执行了一次**（这是 Effect 的约定，和 `Computed` 一样）。

输出：

```text
   停止前 ticks = 3     ← 构造 1 次 + counter=1 + counter=2
   停止后 ticks = 3, active = false
```

`counter = 3` 之后 ticks 没有变成 4 —— `ticker.stop()` 把这条 Effect 摘除了。

三个要点：

- `stop()` 立即生效，之后的依赖变化不再触发；
- `active()` 返回当前状态，便于断言；
- 不调用 `stop()` 也可以 —— Effect 析构时自动停止（这就是后面第 6 章要讲的 RAII 思路）。

---

## 🧭 三者怎么选

| 你想做的事 | 用什么 |
|---|---|
| 一次改多个字段，只要最终结果 | `batch(fn)` |
| 读值做日志/调试，不要它成为依赖 | `reactive::untracked(fn)` |
| 执行副作用（写文件、打接口、驱动老控件） | `Effect` |
| 算出一个新值给别人用 | `Computed`（第 4 章） |

---

## 📌 小结

- 🛑 `batch` 把一批写入合并成一次推送，**中间态不再出现在界面上**；
- 👻 `untracked` 读值但不登记依赖，专治"顺手读了一下结果被反复触发"；
- ⚙️ `Effect` 是纯副作用对象，构造即执行，`stop()` 停止，析构自动收尾；
- 🧠 记住区分：**`Computed` 产生值，`Effect` 产生动作**。

---

**上一章** 👈 [第 4 章 Computed 的魔法：自动依赖追踪](04-Computed的魔法-自动依赖追踪.md)
**下一章** 👉 [第 6 章 Subscription：用作用域表达订阅的生命周期](06-Subscription-生命周期与RAII.md)

> 📂 本章代码来自本仓库 `demos/ch05_batch/main.cpp`，输出为该程序在 Windows / MSVC 19.51 下的真实打印结果。
