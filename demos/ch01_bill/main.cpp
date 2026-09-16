// ch01: 同一份 ViewModel 驱动五个平台, 而它自己不依赖任何 UI。
// 这里用命令行验证这件事: 只改数据, 派生值自动重算。
#include "aria/aria.hpp"

#include <format>
#include <iostream>

using namespace aria;

// 这一段就是"业务逻辑"。它不知道自己是跑在 Qt、iOS、Android 还是浏览器里。
struct BillViewModel {
    Property<double> bill{100.0};  // 账单总额
    Property<int>    people{2};    // 参与人数

    // 依赖不用手写: 首次求值时读到了 bill 和 people, 就自动记下这两条依赖
    Computed<double> per_person{[this] { return bill.get() / people.get(); }};
};

int main() {
    BillViewModel vm;

    // bind: 先立刻用当前值调一次, 之后每次变化再调一次
    auto sub = vm.per_person.bind([](double v) {
        std::cout << std::format("每人应付: {:.2f} 元\n", v);
    });

    std::cout << "-- 改成 4 个人分摊 --\n";
    vm.people = 4;

    std::cout << "-- 总额改成 200 --\n";
    vm.bill = 200.0;

    std::cout << "-- batch: 两处一起改, 只推一次 --\n";
    aria::reactive::batch([&] {
        vm.bill   = 1200.0;
        vm.people = 8;
    });

    return 0;
}
