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
