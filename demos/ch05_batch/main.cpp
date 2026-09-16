// ch05: batch / untracked / Effect -- 精确控制"什么时候通知"
#include "aria/aria.hpp"

#include <iostream>

using namespace aria;

int main() {
    std::cout << "== 1. 逐个改 vs 包进 batch ==\n";
    Property<int>  bill{100};
    Property<int>  people{2};
    Computed<int>  per_person{[&] { return bill.get() / people.get(); }};

    auto sub = per_person.bind([](int v) {
        std::cout << "   每人 = " << v << '\n';
    });

    std::cout << "   -- 逐个改: 每改一次推一次, 中间态会闪 --\n";
    bill = 300;
    people = 4;

    std::cout << "   -- 包进 batch: 只在结束时推一次 --\n";
    aria::batch([&] {
        bill   = 1200;
        people = 8;
    });

    std::cout << "\n== 2. untracked: 读值但不建立依赖 ==\n";
    Property<int> a{0};
    Property<int> b{0};
    int runs = 0;

    Effect e{[&] {
        const int x = a.get();
        const int y = reactive::untracked([&] { return b.get(); });
        ++runs;
        std::cout << "   effect 第 " << runs << " 次: a = " << x << ", b = " << y << '\n';
    }};

    std::cout << "   b = 99 (untracked 读的, 不触发)\n";
    b = 99;

    std::cout << "   a = 1 (追踪到的, 触发)\n";
    a = 1;

    std::cout << "\n== 3. Effect 的停止 ==\n";
    Property<int> counter{0};
    int ticks = 0;

    Effect ticker{[&] {
        counter.get();
        ++ticks;
    }};

    counter = 1;
    counter = 2;
    std::cout << "   停止前 ticks = " << ticks << '\n';

    ticker.stop();
    counter = 3;
    std::cout << "   停止后 ticks = " << ticks
              << ", active = " << (ticker.active() ? "true" : "false") << '\n';

    return 0;
}
