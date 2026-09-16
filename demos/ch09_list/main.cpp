// ch09: ObservableList -- 会通知变化的列表
#include "aria/aria.hpp"

#include <iostream>
#include <string>

using namespace aria;

struct Todo {
    Property<std::string> title;
    Property<bool>        done{false};

    explicit Todo(std::string t) : title{std::move(t)} {}
};

static const char* kind_name(ListChangeKind kind) {
    switch (kind) {
        case ListChangeKind::Insert:      return "Insert";
        case ListChangeKind::Remove:      return "Remove";
        case ListChangeKind::Replace:     return "Replace";
        case ListChangeKind::ItemChanged: return "ItemChanged";
        case ListChangeKind::Reset:       return "Reset";
        case ListChangeKind::Move:        return "Move";
    }
    return "Unknown";
}

int main() {
    ObservableList<Todo> list;

    auto sub = list.observe([&](const ListChange<Todo>& change) {
        std::cout << "   " << kind_name(change.kind) << "  index = " << change.index;
        if (change.kind == ListChangeKind::Move) {
            std::cout << "  from_index = " << change.from_index;
        }
        if (change.item) {
            std::cout << "  title = " << change.item->title.get();
        }
        std::cout << '\n';
    });

    std::cout << "== 1. 增删改移都会产生事件 ==\n";
    auto first = list.emplace_back("写教程");
    list.emplace_back("跑 demo");
    list.push_back(std::make_shared<Todo>("发 GitHub"));

    std::cout << "   -- 元素内部变化 --\n";
    first->done = true;

    std::cout << "   -- 移动 --\n";
    list.move(2, 0);

    std::cout << "   -- 删除 --\n";
    list.remove_at(1);

    std::cout << "   -- 替换 --\n";
    list.replace_at(0, std::make_shared<Todo>("改标题"));

    std::cout << "\n== 2. 元素自身也是响应式的 ==\n";
    auto watcher = first->done.bind([](bool v) {
        std::cout << "   第一项完成状态 -> " << (v ? "true" : "false") << '\n';
    });
    first->done = false;
    first->done = true;

    std::cout << "\n== 3. 快照与遍历 ==\n";
    std::cout << "   当前大小 = " << list.size() << '\n';
    for (const auto& item : list.items()) {
        std::cout << "   - " << item->title.get() << '\n';
    }

    std::cout << "\n== 4. clear 产生 Reset ==\n";
    list.clear();
    std::cout << "   清空后大小 = " << list.size() << '\n';

    return 0;
}
