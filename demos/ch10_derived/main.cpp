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
