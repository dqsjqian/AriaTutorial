// ch03: Property 精讲 -- 读写、追踪与等值门
#include "aria/aria.hpp"

#include <iostream>
#include <vector>

using namespace aria;

int main() {
    std::cout << "== 1. 等值门: 写入相同值不产生任何通知 ==\n";
    Property<int> p{10};
    auto s1 = p.on_changed([](const int& v) {
        std::cout << "   on_changed -> " << v << '\n';
    });
    std::cout << "   p = 20\n";
    p = 20;
    std::cout << "   p = 20 (写入相同值)\n";
    p = 20;
    std::cout << "   p = 30\n";
    p = 30;

    std::cout << "\n== 2. bind 与 on_changed 的区别: bind 会先同步一次 ==\n";
    auto s2 = p.bind([](const int& v) {
        std::cout << "   bind -> " << v << '\n';
    });
    std::cout << "   p = 40\n";
    p = 40;

    std::cout << "\n== 3. observe: 同时拿到旧值和新值 ==\n";
    auto s3 = p.observe([](const int& old_v, const int& new_v) {
        std::cout << "   observe -> " << old_v << " 变成 " << new_v << '\n';
    });
    std::cout << "   p = 50\n";
    p = 50;

    std::cout << "\n== 4. mutate: 原地改容器, 不做等值比较 ==\n";
    Property<std::vector<int>> items{std::vector<int>{1, 2, 3}};
    auto s4 = items.on_changed([](const std::vector<int>& v) {
        std::cout << "   元素个数 -> " << v.size() << '\n';
    });
    std::cout << "   items.mutate(尾部追加 4)\n";
    items.mutate([](std::vector<int>& v) { v.push_back(4); });

    std::cout << "\n== 5. peek: 读值但不建立依赖 ==\n";
    Property<int> base{7};
    Computed<int> tracked([&] { return base.get() * 2; });   // 依赖 base
    Computed<int> frozen([&] { return base.peek() * 2; });   // 不依赖 base
    std::cout << "   tracked = " << tracked.get() << ", frozen = " << frozen.get() << '\n';
    std::cout << "   base = 100\n";
    base = 100;
    std::cout << "   tracked = " << tracked.get() << " (依赖失效, 自动重算)\n";
    std::cout << "   frozen  = " << frozen.get() << " (peek 不建依赖, 缓存没失效)\n";

    return 0;
}
