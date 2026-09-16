// ch06: Subscription -- 用作用域表达订阅的生命周期
#include "aria/aria.hpp"

#include <iostream>

using namespace aria;

int main() {
    std::cout << "== 1. 离开作用域, 订阅自动断开 ==\n";
    Property<int> p{0};
    {
        auto sub = p.on_changed([](const int& v) {
            std::cout << "   收到 " << v << '\n';
        });
        p = 1;
        std::cout << "   作用域内 active = " << (sub.active() ? "true" : "false") << '\n';
    }
    p = 2;
    std::cout << "   (离开作用域后 p = 2 没有任何输出)\n";

    std::cout << "\n== 2. release(): 提前主动断开 ==\n";
    auto sub2 = p.on_changed([](const int& v) {
        std::cout << "   收到 " << v << '\n';
    });
    p = 3;
    sub2.release();
    std::cout << "   release 后 active = " << (sub2.active() ? "true" : "false") << '\n';
    p = 4;
    std::cout << "   (p = 4 没有输出)\n";

    std::cout << "\n== 3. SubscriptionBag: 一次性收拢, 反序释放 ==\n";
    {
        SubscriptionBag bag;
        bag += Subscription{[] { std::cout << "   断开第 1 条\n"; }};
        bag += Subscription{[] { std::cout << "   断开第 2 条\n"; }};
        bag += Subscription{[] { std::cout << "   断开第 3 条\n"; }};
        std::cout << "   bag 内订阅数 = " << bag.size() << '\n';
        std::cout << "   离开作用域:\n";
    }

    std::cout << "\n== 4. 装进成员: 对象销毁时一次性拆干净 ==\n";
    struct Panel {
        SubscriptionBag bag;
        Subscription   keep_alive;

        explicit Panel(Property<int>& source) {
            bag += source.on_changed([](const int& v) {
                std::cout << "   panel 收到 " << v << '\n';
            });
            keep_alive = source.bind([](const int& v) {
                std::cout << "   panel 绑定 " << v << '\n';
            });
        }
    };

    {
        Panel panel{p};
        p = 5;
        std::cout << "   panel 析构:\n";
    }
    p = 6;
    std::cout << "   (panel 销毁后 p = 6 没有输出)\n";

    return 0;
}
