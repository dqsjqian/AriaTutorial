// ch13: ViewModel 生命周期 -- 激活、父子树、销毁钩子
//
// ViewModel 在 aria::binding 命名空间, 不是 aria:: 下的。
#include "aria/aria.hpp"
#include "aria/binding/view_model.hpp"

#include <iostream>
#include <memory>

using namespace aria;
using namespace aria::binding;

class CounterVm : public ViewModel {
public:
    Property<int> count{0};
    int           activate_count   = 0;
    int           deactivate_count = 0;
    const char*   label            = "";

    explicit CounterVm(const char* name) : label{name} {}

protected:
    void on_activate() override {
        ++activate_count;
        std::cout << "   [" << label << "] on_activate  (第 " << activate_count << " 次)\n";
    }
    void on_deactivate() override {
        ++deactivate_count;
        std::cout << "   [" << label << "] on_deactivate (第 " << deactivate_count << " 次)\n";
    }
};

int main() {
    std::cout << "== 1. activate / deactivate 是幂等的 ==\n";
    auto vm = std::make_shared<CounterVm>("counter");
    std::cout << "   初始 is_active = " << (vm->is_active().get() ? "true" : "false") << '\n';

    vm->activate();
    vm->activate();
    vm->activate();
    std::cout << "   连续 activate 三次, on_activate 实际执行 "
              << vm->activate_count << " 次\n";

    std::cout << "\n== 2. 子 VM 随父 VM 一起激活 ==\n";
    auto parent = std::make_shared<CounterVm>("parent");
    auto child  = std::make_shared<CounterVm>("child");
    parent->add_child(child);
    parent->activate();
    std::cout << "   父激活次数 = " << parent->activate_count
              << ", 子激活次数 = " << child->activate_count << '\n';

    std::cout << "\n== 3. track: 把订阅挂在 VM 上, VM 销毁时自动断开 ==\n";
    Property<int> source{0};
    auto tracked = std::make_shared<CounterVm>("tracked");
    tracked->track(source.on_changed([](const int& v) {
        std::cout << "   tracked 收到 " << v << '\n';
    }));

    std::cout << "   source = 1\n";
    source = 1;

    std::cout << "   tracked VM 销毁 ...\n";
    tracked.reset();

    std::cout << "   source = 2\n";
    source = 2;
    std::cout << "   (销毁后不再有输出)\n";

    std::cout << "\n== 4. destroy hook 按后进先出执行 ==\n";
    {
        auto hooked = std::make_shared<CounterVm>("hooked");
        hooked->add_destroy_hook([] { std::cout << "   hook 1 执行\n"; });
        hooked->add_destroy_hook([] { std::cout << "   hook 2 执行\n"; });
        hooked->add_destroy_hook([] { std::cout << "   hook 3 执行\n"; });
        std::cout << "   注册了 3 个 hook, 现在销毁 VM:\n";
    }

    return 0;
}
