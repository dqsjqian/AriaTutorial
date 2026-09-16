# Derived Views: Filtering, Sorting, Deduplication, Paging

Once you have a list, four operations always come up: **show a subset, sort by a field, deduplicate, paginate** 🔍

In Aria all four are *views*, not copies. The difference matters: a view **follows its source automatically**, so you never recompute it when the data changes.

---

## ⚠️ One thing to get right first

These derived views are **not part of the `<aria/aria.hpp>` umbrella** — include them explicitly:

```cpp
#include "aria/derived/distinct_list.hpp"
#include "aria/derived/filtered_list.hpp"
#include "aria/derived/paged_list.hpp"
#include "aria/derived/sorted_list.hpp"
```

The factory functions (`filtered` / `sorted` / `distinct` / `paged`) live in namespace `aria::`.

The umbrella header carries only the core — `Property` / `Computed` / `Command` / `ObservableList` / `Validator`. Derived views are deliberately separate so the umbrella does not drag in everything.

---

## 📄 The complete program

```cpp
// ch10: 派生视图 -- 筛选、排序、去重、分页的组合
//
// 注意: 这几个派生视图不在 <aria/aria.hpp> 这把伞里, 需要单独包含。
// 工厂函数 (filtered / sorted / distinct / paged) 位于 aria:: 命名空间。
#include "aria/aria.hpp"
#include "aria/derived/distinct_list.hpp"
#include "aria/derived/filtered_list.hpp"
#include "aria/derived/paged_list.hpp"
#include "aria/derived/sorted_list.hpp"

#include <iostream>
#include <memory>

using namespace aria;

struct Row {
    int value = 0;
    explicit Row(int v) : value{v} {}
};

template <typename List>
static void dump(const char* name, const List& list) {
    std::cout << "   " << name << " =";
    for (std::size_t i = 0; i < list.size(); ++i) {
        std::cout << ' ' << list.at(i)->value;
    }
    std::cout << '\n';
}

int main() {
    auto source = std::make_shared<ObservableList<Row>>();
    for (int v : {5, 2, 8, 2, 9, 1}) {
        source->push_back(std::make_shared<Row>(v));
    }
    dump("源列表     ", *source);

    std::cout << "\n== 1. FilteredList: 只留下偶数 ==\n";
    auto evens = filtered(source, [](const Row& r) { return r.value % 2 == 0; });
    dump("evens      ", *evens);
    std::cout << "   新插入一个奇数 7 ...\n";
    source->push_back(std::make_shared<Row>(7));
    dump("evens      ", *evens);
    std::cout << "   再插入一个偶数 4 ...\n";
    source->push_back(std::make_shared<Row>(4));
    dump("evens      ", *evens);

    std::cout << "\n== 2. SortedList: 按值升序 ==\n";
    auto ascending = sorted(evens, [](const Row& a, const Row& b) {
        return a.value < b.value;
    });
    dump("ascending  ", *ascending);

    std::cout << "\n== 3. DistinctList: 去重 ==\n";
    auto unique = distinct<int>(source, [](const Row& r) { return r.value; });
    dump("unique     ", *unique);

    std::cout << "\n== 4. PagedList: 每页 2 条 ==\n";
    auto page = paged(ascending, 2, 0);
    dump("page 0     ", *page);
    page->page_index().set(1);
    dump("page 1     ", *page);
    page->page_index().set(2);
    dump("page 2     ", *page);

    std::cout << "\n== 5. 链式组合: 源 -> 筛选 -> 排序 -> 分页 ==\n";
    auto chained = paged(
        sorted(filtered(source, [](const Row& r) { return r.value >= 5; }),
               [](const Row& a, const Row& b) { return a.value > b.value; }),
        2, 0);
    dump("降序且 >=5  ", *chained);

    std::cout << "\n   改动源列表, 整条链自动更新 ...\n";
    source->push_back(std::make_shared<Row>(100));
    dump("降序且 >=5  ", *chained);

    return 0;
}
```

**Actual output**:

```text
   源列表      = 5 2 8 2 9 1

== 1. FilteredList: 只留下偶数 ==
   evens       = 2 8 2
   新插入一个奇数 7 ...
   evens       = 2 8 2
   再插入一个偶数 4 ...
   evens       = 2 8 2 4

== 2. SortedList: 按值升序 ==
   ascending   = 2 2 4 8

== 3. DistinctList: 去重 ==
   unique      = 5 2 8 9 1 7 4

== 4. PagedList: 每页 2 条 ==
   page 0      = 2 2
   page 1      = 4 8
   page 2      =

== 5. 链式组合: 源 -> 筛选 -> 排序 -> 分页 ==
   降序且 >=5   = 9 8

   改动源列表, 整条链自动更新 ...
   降序且 >=5   = 100 9
```

---

## 1️⃣ FilteredList: the source changes, the view follows

```cpp
auto evens = filtered(source, [](const Row& r) { return r.value % 2 == 0; });
```

Source `5 2 8 2 9 1` filters to `2 8 2`. Insert an odd 7:

```text
   新插入一个奇数 7 ...
   evens       = 2 8 2       ← unchanged
```

Insert an even 4:

```text
   再插入一个偶数 4 ...
   evens       = 2 8 2 4     ← appears automatically
```

**You wrote no "re-filter when the source changes" code** ✅ That is what *view* means: it subscribes to the source's events and decides whether each one is relevant.

Note also that inserting an odd number did **not** rebuild the view — the event was simply ignored. That is precisely where a view beats "recompute everything".

---

## 2️⃣ SortedList

```cpp
auto ascending = sorted(evens, [](const Row& a, const Row& b) {
    return a.value < b.value;
});
```

`evens` is `2 8 2 4`; sorted it becomes `2 2 4 8`. The comparator is `bool(const T&, const T&)`, matching `std::sort`'s convention — nothing new to learn.

---

## 3️⃣ DistinctList

```cpp
auto unique = distinct<int>(source, [](const Row& r) { return r.value; });
```

Source `5 2 8 2 9 1` (plus the later 7 and 4) deduplicates to `5 2 8 9 1 7 4`.

The first template parameter is the **key type** (`int`); the second extracts the key from an element. The explicit template argument is needed because the key type cannot be deduced from the lambda.

---

## 4️⃣ PagedList: paging is reactive

```cpp
auto page = paged(ascending, 2, 0);
page->page_index().set(1);
```

Note `page_index()` returns **a `Property`** — the current page is itself a reactive state:

```text
   page 0      = 2 2
   page 1      = 4 8
   page 2      =            ← out of range, empty
```

So you can bind it straight to previous/next buttons or to a URL parameter, instead of maintaining "current page" separately and filtering by hand.

---

## 5️⃣ Chaining: a data pipeline

```cpp
auto chained = paged(
    sorted(filtered(source, [](const Row& r) { return r.value >= 5; }),
           [](const Row& a, const Row& b) { return a.value > b.value; }),
    2, 0);
```

One expression builds **source → filter (≥5) → descending sort → page (2 per page)**.

```text
   降序且 >=5   = 9 8
   改动源列表, 整条链自动更新 ...
   降序且 >=5   = 100 9
```

Appending `100` to the source made the chain recompute itself: it passes the filter, sorts to the front, and the first page becomes `100 9`.

The factory signature table:

| Factory | Signature |
|---|---|
| `filtered(source, pred)` | `pred: bool(const T&)` |
| `sorted(source, cmp)` | `cmp: bool(const T&, const T&)` |
| `distinct<Key>(source, key_of)` | `key_of: Key(const T&)` |
| `paged(source, size, index = 0)` | page size and initial page |

---

## 💡 Views versus copies

Think of a derived view as a *query* rather than a *result*:

- **Zero copy** — a view keeps indices and ordering, not element copies;
- **Incremental** — one insert into the source is one insert to process, not a full recompute;
- **Cost grows with chain length** — every source change walks the whole chain. Chapter 11's `reconcile` offers a more radical alternative.

---

## 📌 Summary

- 📦 Derived views need explicit `#include "aria/derived/..."`; they are not in `aria.hpp`;
- 🔍 The four factories live in `aria::` and return `shared_ptr`;
- 🔄 Views subscribe to their source — **no manual refresh code**;
- 📄 `paged`'s `page_index()` is a `Property`, so paging participates in reactivity;
- 🔗 Factories chain freely: source → filter → sort → page.

---

**Previous chapter** 👈 [Chapter 9 — ObservableList: A List That Announces Its Changes](09-observable-list.en.md)
**Next chapter** 👉 [Chapter 11 — reconcile: Refreshing a List from Fresh Data](11-reconcile.en.md)

> 📂 Code from `demos/ch10_derived/main.cpp`. Output is the program's real stdout on Windows / MSVC 19.51.
