// ch11: reconcile -- 用一份新数据刷新列表, 只发出必要的变更
#include "aria/aria.hpp"

#include <iostream>
#include <memory>
#include <vector>

using namespace aria;

struct Row {
    int value = 0;
    explicit Row(int v) : value{v} {}
};

struct ByValue {
    int operator()(const Row& r) const { return r.value; }
};

static void dump(const ObservableList<Row>& list) {
    std::cout << "   列表 =";
    for (const auto& item : list.items()) {
        std::cout << ' ' << item->value;
    }
    std::cout << '\n';
}

static std::vector<std::shared_ptr<Row>> make(std::initializer_list<int> values) {
    std::vector<std::shared_ptr<Row>> out;
    out.reserve(values.size());
    for (int v : values) {
        out.push_back(std::make_shared<Row>(v));
    }
    return out;
}

int main() {
    ObservableList<Row> list;
    for (auto& row : make({1, 2, 3})) {
        list.push_back(row);
    }

    std::size_t inserts = 0, removes = 0, moves = 0, resets = 0;
    auto sub = list.observe([&](const ListChange<Row>& change) {
        switch (change.kind) {
            case ListChangeKind::Insert: ++inserts; break;
            case ListChangeKind::Remove: ++removes; break;
            case ListChangeKind::Move:   ++moves;   break;
            case ListChangeKind::Reset:  ++resets;  break;
            default: break;
        }
    });

    std::cout << "== 1. 数据没变: 一个事件都不发 ==\n";
    dump(list);
    std::size_t events = list.reconcile(make({1, 2, 3}), ByValue{});
    std::cout << "   reconcile 返回事件数 = " << events
              << " (insert=" << inserts << " remove=" << removes
              << " move=" << moves << " reset=" << resets << ")\n";

    std::cout << "\n== 2. 只追加一条 ==\n";
    events = list.reconcile(make({1, 2, 3, 4}), ByValue{});
    dump(list);
    std::cout << "   事件数 = " << events
              << " (insert=" << inserts << " remove=" << removes
              << " move=" << moves << " reset=" << resets << ")\n";

    std::cout << "\n== 3. 重新排序 ==\n";
    events = list.reconcile(make({4, 1, 3, 2}), ByValue{});
    dump(list);
    std::cout << "   事件数 = " << events
              << " (insert=" << inserts << " remove=" << removes
              << " move=" << moves << " reset=" << resets << ")\n";

    std::cout << "\n== 4. 删掉两条 ==\n";
    events = list.reconcile(make({4, 3}), ByValue{});
    dump(list);
    std::cout << "   事件数 = " << events
              << " (insert=" << inserts << " remove=" << removes
              << " move=" << moves << " reset=" << resets << ")\n";

    std::cout << "\n== 5. 出现重复键: 降级为 Reset ==\n";
    events = list.reconcile(make({7, 7, 8}), ByValue{});
    dump(list);
    std::cout << "   事件数 = " << events
              << " (insert=" << inserts << " remove=" << removes
              << " move=" << moves << " reset=" << resets << ")\n";

    std::cout << "\n   重复键会让增量对齐无法判断谁是谁, 于是整表重建。\n";
    std::cout << "   这也是为什么 key_of 必须保证唯一。\n";

    return 0;
}
