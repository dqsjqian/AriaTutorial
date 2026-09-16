// ch07: 两个必踩的坑 -- 幽灵依赖与循环依赖
#include "aria/aria.hpp"

#include <iostream>
#include <string>

using namespace aria;

int main() {
    std::cout << "== 1. 幽灵依赖: 当前分支没读到的值不算依赖 ==\n";
    Property<int>  a{1};
    Property<int>  b{2};
    Property<bool> use_a{true};

    int recomputes = 0;
    Computed<int> chosen{[&] {
        ++recomputes;
        return use_a.get() ? a.get() : b.get();
    }};

    std::cout << "   初值 = " << chosen.get()
              << ", 重算次数 = " << recomputes << '\n';

    b = 20;
    std::cout << "   改 b (当前分支没读到): 重算次数 = " << recomputes
              << ", 值 = " << chosen.get() << '\n';

    a = 10;
    std::cout << "   改 a (当前分支读到了): 重算次数 = " << recomputes
              << ", 值 = " << chosen.get() << '\n';

    std::cout << "\n== 2. 循环依赖: 会被框架熔断 ==\n";
    Property<int> p{0};
    Property<int> q{0};
    p.set_debug_name("p");
    q.set_debug_name("q");

    bool live = false;
    Effect e_pq{[&] {
        const int v = p.get();
        if (live) {
            q.set(v + 1);
        }
    }};
    Effect e_qp{[&] {
        const int v = q.get();
        if (live) {
            p.set(v + 1);
        }
    }};

    live = true;
    try {
        p.set(1);
        std::cout << "   没有触发熔断\n";
    } catch (const CircularDependencyError& ex) {
        const std::string message = ex.what();
        std::cout << "   捕获到 CircularDependencyError\n";
        std::cout << "   消息长度 = " << message.size() << " 字符\n";
    }

    std::cout << "\n== 3. 正确做法: 打破环 ==\n";
    Property<int> a1{1};
    Property<int> b1{0};
    Computed<int> doubled{[&] { return a1.get() * 2; }};   // 单向: a1 -> doubled

    auto sub = doubled.on_changed([&](const int& v) {
        b1 = v + 1;                                        // 只在外层订阅里改, 不参与依赖追踪
        std::cout << "   doubled = " << v << ", b1 = " << b1.get() << '\n';
    });

    a1 = 5;
    a1 = 6;

    return 0;
}
