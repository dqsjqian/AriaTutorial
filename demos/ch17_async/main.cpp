// ch17: 协程、并发与取消
//
// 一个关键区分:
//   同步任务体 (没有真正切线程) -> 可以直接 blocking_get()
//   真异步任务体 (schedule_on 切了线程) -> 必须 start_detached() + 自己等
// blocking_get 的注释里写得很明白: "only safe if the task body is
// synchronous (no real async)"。混用会直接崩。
#include "aria/aria.hpp"
#include "aria/async/cancellation.hpp"
#include "aria/async/executor.hpp"
#include "aria/async/task.hpp"
#include "aria/async/when_all.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace aria;
using namespace aria::async;

/// 同步任务体: 不切线程, 直接就能取出结果。
Task<int> square(int x) {
    co_return x * x;
}

/// 真异步: 先切到线程池再干活。
Task<int> square_on_pool(ThreadPoolExecutor& pool, int x, int delay_ms) {
    co_await schedule_on(pool);
    std::this_thread::sleep_for(std::chrono::milliseconds{delay_ms});
    co_return x * x;
}

/// 轮询等待一个原子标志, 模拟真实程序里的主循环。
static void wait_for(const std::atomic<bool>& flag) {
    while (!flag.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
}

int main() {
    std::cout << "== 1. 同步任务体: blocking_get 直接取结果 ==\n";
    std::cout << "   square(7) = " << square(7).blocking_get() << '\n';

    std::cout << "\n== 2. 真异步: start_detached + 自己等 ==\n";
    ThreadPoolExecutor pool{4};

    std::atomic<int>  single{0};
    std::atomic<bool> single_done{false};

    // 注意: 协程 lambda 绝不按引用捕获 [&]。闭包对象在语句结束就销毁,
    // 而协程帧活得比它久, resume 时通过悬垂引用访问捕获变量是 UB
    // (GCC -O2 直接崩, clang 只是碰巧能跑)。状态一律走参数传引用,
    // 引用本身是拷贝进协程帧的, 指向 main 的栈, 生命周期由 main 保证。
    auto one = [](std::atomic<int>& out, std::atomic<bool>& done,
                  ThreadPoolExecutor& pool) -> Task<void> {
        out = co_await square_on_pool(pool, 9, 10);
        done = true;
    }(single, single_done, pool);
    std::move(one).start_detached();

    wait_for(single_done);
    std::cout << "   square_on_pool(9) = " << single.load() << '\n';

    std::cout << "\n== 3. when_all: 三个子任务一起等 ==\n";
    std::atomic<int>  total{0};
    std::atomic<bool> parallel_done{false};

    const auto started = std::chrono::steady_clock::now();

    auto parallel = [](std::atomic<int>& out, std::atomic<bool>& done, ThreadPoolExecutor& pool) -> Task<void> {
        auto [a, b, c] = co_await when_all(
            square_on_pool(pool, 1, 60),
            square_on_pool(pool, 2, 60),
            square_on_pool(pool, 3, 60));
        out = a + b + c;
        done = true;
    }(total, parallel_done, pool);
    std::move(parallel).start_detached();

    wait_for(parallel_done);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count();

    std::cout << "   1^2 + 2^2 + 3^2 = " << total.load() << '\n';
    std::cout << "   三个各睡 60ms 的任务, 实测总耗时 " << elapsed
              << "ms (并发执行; 串行的话约 180ms)\n";

    std::cout << "\n== 4. 取消令牌 ==\n";
    CancellationSource source;
    auto token = source.token();

    std::cout << "   初始   is_cancelled = "
              << (token.is_cancelled() ? "true" : "false") << '\n';

    source.cancel();
    std::cout << "   取消后 is_cancelled = "
              << (token.is_cancelled() ? "true" : "false") << '\n';

    try {
        token.throw_if_cancelled();
        std::cout << "   throw_if_cancelled 没有抛异常\n";
    } catch (const OperationCancelled&) {
        std::cout << "   throw_if_cancelled 抛出 OperationCancelled\n";
    }

    std::cout << "\n== 5. 两个独立令牌互不影响 ==\n";
    CancellationSource parent;
    CancellationSource child;

    child.cancel();
    std::cout << "   子令牌已取消 = " << (child.token().is_cancelled() ? "true" : "false") << '\n';
    std::cout << "   父令牌已取消 = " << (parent.token().is_cancelled() ? "true" : "false")
              << " (父没被波及)\n";

    parent.cancel();
    std::cout << "   父令牌取消后 = " << (parent.token().is_cancelled() ? "true" : "false") << '\n';

    // 给线程池一点时间收尾, 再让它析构。
    std::this_thread::sleep_for(std::chrono::milliseconds{50});

    return 0;
}