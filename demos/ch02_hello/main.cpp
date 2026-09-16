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
