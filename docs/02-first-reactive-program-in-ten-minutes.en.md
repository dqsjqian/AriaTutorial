# First Reactive Program in Ten Minutes

> The code in this chapter is reproduced verbatim from `demos/ch02_hello/main.cpp`.
> Source comments are in Chinese, matching the repository.

The previous chapter explained what problem Aria exists to solve. This one actually runs it — from nothing to visible output, **in well under ten minutes** ⏱️

## 📋 Requirements

| Item | Requirement |
|---|---|
| CMake | >= 3.20 |
| Compiler | MSVC v143+ (VS 2022/2026) / GCC 12+ / Clang 15+ |
| C++ standard | C++20 minimum; `-DCMAKE_CXX_STANDARD=23` for C++23 |
| Other dependencies | None. This chapter's demo only needs `aria::core` |

**Why the floor is C++20** 🤔 Aria uses full coroutines and concepts, so C++17 cannot compile it. This is not a style preference — the entire `aria::async` layer is built on coroutines.

---

## 1️⃣ Get Aria

Pick either route.

**Option A — source tree** (recommended if you want to read the framework while learning)

```bash
git clone https://github.com/dqsjqian/Aria.git
```

**Option B — install as an SDK** (recommended for production projects)

```bash
cmake -S Aria -B Aria/build/release \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build Aria/build/release -j
cmake --install Aria/build/release
```

After installing, your own project needs just one line:

```cmake
find_package(aria 2.0 CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE aria::core)
```

The only difference between the two routes is how Aria is found: option A pulls the source tree in via `add_subdirectory`, option B links an installed library. This repository's `CMakeLists.txt` supports both — it tries `find_package` first and falls back to the source tree you pass via `-DARIA_ROOT=`.

---

## 2️⃣ Configure and build

```bash
git clone https://github.com/dqsjqian/AriaTutorial.git
cd AriaTutorial

# Option A: source tree
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DARIA_ROOT=../Aria

# Option B: Aria already installed
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

cmake --build build -j
```

> ⚠️ **Windows note**: the `.ps1` scripts and `cmake` both need a ready compiler environment. Open **Developer PowerShell for VS**, or make sure `cmake` and `cl.exe` are on `PATH`. A plain PowerShell window will most likely report that no compiler was found.

A successful configure prints Aria's build summary. Check these lines:

```text
-- aria v2.0.0 configuration:
--   Platform        : Windows
--   Build type      : Release
--   C++ standard    : 20
--   Compiler        : MSVC 19.51.36247.0
```

Executables all land in `build/bin/`:

```bash
./build/bin/ch02_hello
```

---

## 3️⃣ The first program

Here is the complete program, which is also `demos/ch02_hello/main.cpp`:

```cpp
// ch02_hello: Aria 最小可运行程序
// 演示 Property / Computed / Command / Subscription 四件套的协作
#include "aria/aria.hpp"

#include <iostream>
#include <string>

using namespace aria;

int main() {
    // 1. Property: 可读写的状态, 初值 0
    Property<int> count{0};

    // 2. Computed: 只读派生值。依赖不用手写 -- 首次求值时读到了
    //    count, 就自动记下这个依赖。
    Computed<std::string> label([&] {
        return "count = " + std::to_string(count.get());
    });

    // 3. Command: 把"一个动作"包成对象, 可以被 UI 直接绑定
    Command<> increment([&] { count = count.get() + 1; });

    // 4. bind 建立订阅: 先立刻用当前值调一次, 之后每次变化都再调一次。
    //    返回值必须接住 -- 它决定订阅活多久。
    auto sub = label.bind([](const std::string& s) {
        std::cout << s << '\n';
    });

    std::cout << "-- 连续执行两次 increment --\n";
    increment();
    increment();

    std::cout << "-- 主动断开订阅 --\n";
    sub.release();

    increment();
    std::cout << "-- 断开后 count 实际值 = " << count.get() << " (但没有再打印)\n";
    return 0;
}
```

Notice there is **no UI code at all** 🎯 No `QApplication`, no window, no event loop. The whole reactive system runs inside `main` — which is exactly the development style Aria wants for you: **verifying business logic never depends on a UI**.

---

## 🔍 Walking through it

### Property — read/write state

```cpp
Property<int> count{0};
```

`Property` is an observable node in the reactive graph. It looks like a variable with an initial value, but assigning to it does one extra thing: **it notifies every downstream node that depends on it**.

```cpp
count = count.get() + 1;   // assignment → downstream recomputes
```

`get()` is not decorative. Its job is **registering a dependency while reading** — whoever read `count` during their evaluation becomes downstream of `count`.

### Computed — a read-only derived value

```cpp
Computed<std::string> label([&] {
    return "count = " + std::to_string(count.get());
});
```

The best part is what is missing: **there is no dependency list** 🎉 If you have written Vue's `computed`, or React's `useMemo` with its `[count]` array, you know how easy it is to miss one entry and silently break the memo.

Aria's `Computed` infers dependencies from "what did it read while evaluating": this evaluation read `count`, so `count` is a dependency. **You cannot forget to list it, because you never list it.**

### Command — an action as an object

```cpp
Command<> increment([&] { count = count.get() + 1; });
increment();   // callable like a function
```

`Command<>` wraps an action so a UI can bind to it (`engine.bind_command(...)`). What it adds over a bare lambda is **an executable state** — a button's enabled/disabled can be driven by `can_execute`. Chapter 8 covers that.

### bind — establishing a subscription

```cpp
auto sub = label.bind([](const std::string& s) {
    std::cout << s << '\n';
});
```

`bind` does two things: it calls the callback **immediately with the current value** (which is why the first output line is `count = 0`), then calls it again **on every change**.

---

## 🖥️ Running it

```text
count = 0
-- 连续执行两次 increment --
count = 1
count = 2
-- 主动断开订阅 --
-- 断开后 count 实际值 = 3 (但没有再打印)
```

Line by line:

| Where | What happened |
|---|---|
| `count = 0` | `bind`'s initial sync; no `increment` has run yet |
| `count = 1` | First `increment()` → `count` becomes 1 → `label` invalidated and recomputed → pushed |
| `count = 2` | Second `increment()`, same path |
| Nothing after release | `sub.release()` unsubscribed; the later `increment()` still moves `count` to 3, but nobody is listening |
| `count 实际值 = 3` | Reading via `get()` proves the value did change — it simply had no observer |

---

## ⚠️ One thing that must be clear: `bind`'s return value

In the code above:

```cpp
auto sub = label.bind(...);   // correct: keep the handle
```

But if you write:

```cpp
label.bind(...);              // wrong: the subscription dies instantly
```

**The subscription ends on that very line and nothing is ever printed** 😱

Why: the `Subscription` returned by `bind` *is* the lifetime handle for that subscription. When it is destroyed, the subscription is torn down. Without a variable to receive it, the temporary is destroyed at the semicolon — and the subscription ends right there.

Aria guards against this: `bind` is marked `[[nodiscard]]`, so that mistake produces a **compiler warning** rather than a silent failure 🛡️

There is a real benefit to this design in UI code: `Subscription` is normally stored as a member of the View. When the View is destroyed, the member is destroyed and the subscription is released — **so it can never call back into an already-destroyed widget**. Chapter 6 builds on exactly this: "express subscription lifetime with scope".

---

## 🔧 Build troubleshooting

| Symptom | Cause and fix |
|---|---|
| `未找到 Aria` | Neither an SDK nor `-DARIA_ROOT` was provided. Add `-DARIA_ROOT=<path-to-Aria>`. |
| `CMAKE_CXX_STANDARD` errors | Compiler too old. Confirm GCC 12+ / Clang 15+ / MSVC v143+. |
| `cl.exe` not found (Windows) | Open the terminal via Developer PowerShell for VS. |
| `aria_abi.dll` not found at runtime | Run the executables from `build/bin/`; that directory holds the exe and its runtime libraries together. |
| Garbled Chinese output (MSVC) | This project already passes `/utf-8`; remember to add it to your own project too. |

---

## 📌 Summary

- 🧱 Building takes three steps: get Aria → `cmake -S . -B build` → `cmake --build build`;
- 📦 `Property` is read/write state; `get()` registers a dependency as it reads;
- 🪄 `Computed` discovers its dependencies by evaluating — **no handwritten dependency arrays**;
- 🎬 `Command` turns an action into a bindable object;
- 🔗 `bind` syncs once then subscribes; its return value must be kept, because that handle owns the subscription's lifetime.

---

**Previous chapter** 👈 [Chapter 1 — Why Another C++ MVVM Framework](01-why-another-cpp-mvvm-framework.en.md)
**Next chapter** 👉 [Chapter 3 — Property in Depth: Reading, Tracking, and the Equality Gate](03-property-in-depth.en.md)

> 📂 Code from `demos/ch02_hello/main.cpp`. Output is the program's real stdout on Windows / MSVC 19.51.
