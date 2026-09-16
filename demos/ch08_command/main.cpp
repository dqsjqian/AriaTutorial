// ch08: Command 与 AsyncCommand -- 动作本身也要有状态
#include "aria/aria.hpp"
#include "aria/async/async_command.hpp"
#include "aria/async/executor.hpp"

#include <iostream>

using namespace aria;
using namespace aria::async;

int main() {
    std::cout << "== 1. Command: 谓词决定能不能执行 ==\n";
    Property<bool> agreed{false};
    int submitted = 0;

    Command<> submit(
        [&] { ++submitted; },
        [&] { return agreed.get(); });

    auto sub = submit.observe_can_execute([](bool can) {
        std::cout << "   按钮可点 -> " << (can ? "true" : "false") << '\n';
    });

    submit();
    std::cout << "   未勾选时点击: 提交次数 = " << submitted << '\n';

    agreed = true;
    submit();
    std::cout << "   勾选后点击: 提交次数 = " << submitted << '\n';

    agreed = false;

    std::cout << "\n== 2. Command<Args...>: 带参数的动作 ==\n";
    int captured = 0;
    Command<int> scale([&](int x) { captured = x * 2; });
    scale.execute(21);
    std::cout <<    "   scale(21) -> captured = " << captured << '\n';

    std::cout << "\n== 3. AsyncCommand: 三态 ==\n";
    // ui 用 MainThreadExecutor: 它的任务由本线程 pump, 状态更新不跨线程。
    // worker 用 ThreadPoolExecutor: 真正耗时的活丢到后台线程池。
    // (框架不允许 ui 用 InlineExecutor 而 worker 在别的线程 -- 编译期就会拦下)
    MainThreadExecutor ui;
    ThreadPoolExecutor worker{2};

    AsyncCommand<int, int> twice{
        ui, worker,
        [](int x) -> Task<int> { co_return x * 2; },
        AsyncCommandPolicy::DropIfRunning};

    std::cout << "   执行前 is_executing = "
              << (twice.is_executing.get() ? "true" : "false") << '\n';

    twice.execute(21);
    std::cout << "   刚提交 is_executing = "
              << (twice.is_executing.get() ? "true" : "false") << '\n';

    ui.pump_until([&] { return !twice.is_executing.get(); });

    std::cout << "   执行后 is_executing = "
              << (twice.is_executing.get() ? "true" : "false") << '\n';
    std::cout << "   有错误 = "
              << (twice.last_error.get().has_value() ? "true" : "false") << '\n';
    if (twice.last_result.get().has_value()) {
        std::cout << "   结果 = " << *twice.last_result.get() << '\n';
    }

    return 0;
}
