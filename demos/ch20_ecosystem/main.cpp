// ch20: 综合实战 -- 用 Aria 写一个待办清单
//
// 本章把前面 19 章的东西串起来:
//   Property / Computed / Command / ObservableList / BindingEngine / 内存适配器
// ViewModel 里没有一行界面代码, 界面侧没有一行业务逻辑。
#include "aria/aria.hpp"
#include "aria/binding/binding_engine.hpp"

#include "common/memory_adapter.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace aria;
using namespace aria::binding;
using tutorial::MemoryAdapter;
using tutorial::MemoryView;

struct Todo {
    Property<std::string> title;
    Property<bool>        done{false};

    explicit Todo(std::string t) : title{std::move(t)} {}
};

// ---------------------------------------------------------------------------
// ViewModel: 纯 C++, 可以脱离任何界面做单元测试
// ---------------------------------------------------------------------------
class TodoViewModel {
public:
    Property<std::string> draft{""};        // 输入框里的草稿
    ObservableList<Todo>  todos;            // 待办列表
    Property<int>         remaining{0};     // 未完成条数

    // 有内容才能添加。这个 Computed 会被绑到按钮的 enabled 上。
    Computed<bool> can_add{[&] { return !draft.get().empty(); }};

    Command<> add{[&] {
        auto item = std::make_shared<Todo>(draft.get());
        todos.push_back(std::move(item));
        draft = "";                          // 添加完清空输入框
        refresh_remaining();
    }};

    TodoViewModel() {
        // 列表本身不属于响应式图 (它是 ObservableList 而不是 Property),
        // 所以统计值要在事件回调里手动刷新。
        todos_sub_ = todos.observe([&](const ListChange<Todo>&) {
            refresh_remaining();
        });
    }

    void toggle(std::size_t index) {
        if (index >= todos.size()) {
            return;
        }
        auto item = todos.at(index);
        item->done = !item->done.get();
        refresh_remaining();
    }

    [[nodiscard]] int done_count() const {
        int n = 0;
        for (const auto& item : todos.snapshot()) {
            if (item->done.get()) {
                ++n;
            }
        }
        return n;
    }

private:
    void refresh_remaining() {
        int n = 0;
        for (const auto& item : todos.snapshot()) {
            if (!item->done.get()) {
                ++n;
            }
        }
        remaining = n;
    }

    Subscription todos_sub_;
};

int main() {
    auto adapter = std::make_shared<MemoryAdapter>();
    BindingEngine engine{adapter};

    TodoViewModel vm;

    // ---- 界面侧: 三个控件, 十几行接线 ----
    MemoryView input_view;
    MemoryView add_button;
    MemoryView stats_label;

    engine.bind_text(vm.draft, input_view);
    engine.bind_command(vm.add, add_button);            // 先绑命令 (会写一次 enabled)
    engine.bind_enabled(vm.can_add, add_button);        // 再绑有效状态, 后者权威
    engine.bind_text_projected(vm.remaining, stats_label, [](int n) {
        return "还剩 " + std::to_string(n) + " 项";
    });

    std::cout << "初始状态\n";
    std::cout << "   输入框     = \"" << input_view.text << "\"\n";
    std::cout << "   添加可点击 = " << (add_button.enabled ? "true" : "false") << '\n';
    std::cout << "   统计       = " << stats_label.text << '\n';

    std::cout << "\n-- 用户输入 \"写教程\" --\n";
    input_view.user_type("写教程");
    std::cout << "   VM.draft   = \"" << vm.draft.get() << "\"\n";
    std::cout << "   添加可点击 = " << (add_button.enabled ? "true" : "false") << '\n';

    std::cout << "\n-- 点击添加 --\n";
    add_button.user_click();
    std::cout << "   列表大小   = " << vm.todos.size() << '\n';
    std::cout << "   输入框清空 = \"" << input_view.text << "\"\n";
    std::cout << "   统计       = " << stats_label.text << '\n';
    std::cout << "   添加可点击 = " << (add_button.enabled ? "true" : "false") << '\n';

    std::cout << "\n-- 再添加两条 --\n";
    input_view.user_type("跑 demo");
    add_button.user_click();
    input_view.user_type("发 GitHub");
    add_button.user_click();
    std::cout << "   列表大小   = " << vm.todos.size() << '\n';
    std::cout << "   统计       = " << stats_label.text << '\n';

    std::cout << "\n-- 勾掉第一项 --\n";
    vm.toggle(0);
    std::cout << "   第一项完成 = " << (vm.todos.at(0)->done.get() ? "true" : "false") << '\n';
    std::cout << "   已完成     = " << vm.done_count() << " 项\n";
    std::cout << "   统计       = " << stats_label.text << '\n';

    std::cout << "\n-- 全部勾完 --\n";
    vm.toggle(1);
    vm.toggle(2);
    std::cout << "   统计       = " << stats_label.text << '\n';

    std::cout << "\n同一个 ViewModel 换成 Qt / UIKit / JNI / HTTP 适配器,\n";
    std::cout << "业务逻辑一行都不用改。\n";

    return 0;
}
