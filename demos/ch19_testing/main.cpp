// ch19: 测试 -- ViewModel 是纯 C++, 测试不需要界面
//
// 这一章回答一个问题: 我怎么知道自己的响应式代码是对的?
// 核心优势在于: Aria 的 ViewModel 不依赖任何 UI 工具包, 所以
// 绝大部分逻辑可以在没有窗口、没有事件循环的情况下测完。
#include "aria/aria.hpp"
#include "aria/async/executor.hpp"
#include "aria/async/task.hpp"

#include <iostream>
#include <string>

using namespace aria;
using namespace aria::async;

// ---------------------------------------------------------------------------
// 被测对象: 登录表单
// ---------------------------------------------------------------------------
struct LoginViewModel {
    Property<std::string> user{""};
    Property<std::string> password{""};
    Property<int>         attempts{0};

    Computed<bool> can_submit{[&] {
        return !user.get().empty() && password.get().size() >= 6;
    }};

    Computed<std::string> hint{[&] {
        if (attempts.get() >= 3) return std::string("尝试次数过多");
        if (user.get().empty())   return std::string("请输入用户名");
        if (password.get().size() < 6) return std::string("密码至少 6 位");
        return std::string("可以提交");
    }};
};

// ---------------------------------------------------------------------------
// 极简断言: 教程里够用, 真实项目请用 doctest / Catch2 / GoogleTest
// ---------------------------------------------------------------------------
static int failures = 0;

static void check(bool ok, const char* what) {
    std::cout << (ok ? "   [ok]   " : "   [FAIL] ") << what << '\n';
    if (!ok) {
        ++failures;
    }
}

int main() {
    std::cout << "== 1. 纯逻辑: 没有窗口也能测 ==\n";
    {
        LoginViewModel vm;
        check(!vm.can_submit.get(), "空表单不能提交");

        vm.user = "aria";
        check(!vm.can_submit.get(), "只填用户名仍不能提交");

        vm.password = "12345";
        check(!vm.can_submit.get(), "密码 5 位不能提交");

        vm.password = "123456";
        check(vm.can_submit.get(), "用户名 + 6 位密码可以提交");

        check(vm.hint.get() == "可以提交", "提示文案随状态更新");
    }

    std::cout << "\n== 2. 派生的提示文案覆盖每条分支 ==\n";
    {
        LoginViewModel vm;
        check(vm.hint.get() == "请输入用户名", "空用户名 -> 提示输入用户名");

        vm.user = "aria";
        check(vm.hint.get() == "密码至少 6 位", "密码太短 -> 提示密码长度");

        vm.attempts = 3;
        check(vm.hint.get() == "尝试次数过多", "超过次数 -> 优先级最高的提示");
    }

    std::cout << "\n== 3. 订阅行为: 只在该通知的时候通知 ==\n";
    {
        Property<int> value{0};
        int notifications = 0;
        auto sub = value.on_changed([&](const int&) { ++notifications; });

        value = 1;
        value = 1;              // 等值写入, 不通知
        value = 2;
        check(notifications == 2, "等值写入被静默丢弃");

        sub.release();
        value = 3;
        check(notifications == 2, "退订后不再收到通知");
    }

    std::cout << "\n== 4. 批量修改只产生一次通知 ==\n";
    {
        Property<int> a{0};
        int notifications = 0;
        auto sub = a.on_changed([&](const int&) { ++notifications; });

        aria::batch([&] {
            a = 1;
            a = 2;
            a = 3;
        });
        check(notifications == 1, "batch 内三次写入只推一次");
    }

    std::cout << "\n== 5. 异步逻辑: blocking_get 把协程结果取出来 ==\n";
    {
        auto task = []() -> Task<int> {
            co_return 6 * 7;
        };

        check(task().blocking_get() == 42, "协程结果可以被同步取出");
    }

    std::cout << "\n== 6. 框架自带的一致性套件 ==\n";
    std::cout << "   自定义适配器可以拿框架的套件自查:\n";
    std::cout << "     aria/binding/testing/adapter_conformance.hpp\n";
    std::cout << "   自定义列表源可以跑:\n";
    std::cout << "     aria/testing/list_conformance.hpp\n";
    std::cout << "   这两套是内置适配器和内置列表视图自己也在跑的同一份测试。\n";

    std::cout << "\n------------------------------\n";
    if (failures == 0) {
        std::cout << "全部通过\n";
    } else {
        std::cout << failures << " 项失败\n";
    }
    return failures == 0 ? 0 : 1;
}
