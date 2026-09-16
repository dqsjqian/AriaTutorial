// ch14: BindingEngine -- 把 Property 接到界面
//
// 本章用一个"内存适配器"代替真实控件 (见 common/memory_adapter.hpp),
// 这样在命令行里就能看清绑定的每一条规则。
//
// 注意: 所有 bind_* 都返回 void。订阅由 BindingEngine 自己持有,
//       engine 活着绑定就有效, engine 析构时统一解绑。
#include "aria/aria.hpp"
#include "aria/binding/binding_engine.hpp"
#include "aria/runtime/dispatcher.hpp"

#include "common/memory_adapter.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <string>

using namespace aria;
using namespace aria::binding;
using tutorial::MemoryAdapter;
using tutorial::MemoryView;

static const char* yes_no(bool v) { return v ? "true" : "false"; }

int main() {
    auto adapter = std::make_shared<MemoryAdapter>();
    BindingEngine engine{adapter};

    std::cout << "平台: " << adapter->platform_name() << '\n';

    std::cout << "\n== 1. bind_text: 双向文本 ==\n";
    Property<std::string> name{"Aria"};
    MemoryView name_view;

    engine.bind_text(name, name_view);

    std::cout << "   绑定后 view.text = \"" << name_view.text << "\"\n";
    name = "Hello";
    std::cout << "   VM 改值   -> view.text = \"" << name_view.text << "\"\n";
    name_view.user_type("World");
    std::cout << "   用户输入  -> VM = \"" << name.get() << "\"\n";

    std::cout << "\n== 2. bind_int / bind_double: 数值双向 ==\n";
    Property<int>    count{3};
    Property<double> ratio{0.5};
    MemoryView count_view;
    MemoryView ratio_view;

    engine.bind_int(count, count_view);
    engine.bind_double(ratio, ratio_view);

    std::cout << "   count = " << count.get()
              << " -> view.integer = " << count_view.integer << '\n';
    std::cout << "   ratio = " << ratio.get()
              << " -> view.number  = " << ratio_view.number << '\n';

    count = 7;
    std::cout << "   count = 7 -> view.integer = " << count_view.integer << '\n';

    std::cout << "\n== 3. bind_text_projected: 单向投影 ==\n";
    Property<int> amount{100};
    MemoryView label_view;

    engine.bind_text_projected(amount, label_view, [](int v) {
        return std::string("¥") + std::to_string(v) + ".00";
    });

    std::cout << "   label = \"" << label_view.text << "\"\n";
    amount = 250;
    std::cout << "   amount = 250 -> label = \"" << label_view.text << "\"\n";

    std::cout << "\n== 4. bind_optional_text: 空值也要显示 ==\n";
    Property<std::optional<std::string>> nickname{std::nullopt};
    MemoryView nick_view;

    engine.bind_optional_text(
        nickname, nick_view,
        [](const std::string& s) { return std::string("你好, ") + s; },
        std::string("(还没设置昵称)"));

    std::cout << "   空值 -> \"" << nick_view.text << "\"\n";
    nickname = std::string("谦哥");
    std::cout << "   有值 -> \"" << nick_view.text << "\"\n";

    std::cout << "\n== 5. bind_visible / bind_enabled ==\n";
    Property<bool> loading{true};
    MemoryView panel_view;

    engine.bind_visible(loading, panel_view);

    std::cout << "   loading = true  -> panel.visible = " << yes_no(panel_view.visible) << '\n';
    loading = false;
    std::cout << "   loading = false -> panel.visible = " << yes_no(panel_view.visible) << '\n';

    Property<bool> can_submit{false};
    MemoryView button_view;

    engine.bind_enabled(can_submit, button_view);

    std::cout << "   can_submit = false -> button.enabled = " << yes_no(button_view.enabled) << '\n';
    can_submit = true;
    std::cout << "   can_submit = true  -> button.enabled = " << yes_no(button_view.enabled) << '\n';

    std::cout << "\n== 6. bind_command: 按钮点击 ==\n";
    int clicks = 0;
    Command<> refresh([&] { ++clicks; });
    MemoryView refresh_button;

    engine.bind_command(refresh, refresh_button);

    refresh_button.user_click();
    std::cout << "   点一次   -> clicks = " << clicks << '\n';
    refresh_button.user_click();
    std::cout << "   再点一次 -> clicks = " << clicks << '\n';

    std::cout << "\n== 7. 三种派发策略 ==\n";
    auto dispatcher = std::make_shared<runtime::SimpleDispatcher>();

    auto direct_adapter = std::make_shared<MemoryAdapter>();
    BindingEngine direct_engine{direct_adapter, dispatcher,
                                BindingEngine::DispatchPolicy::Direct};

    auto posted_adapter = std::make_shared<MemoryAdapter>();
    BindingEngine posted_engine{posted_adapter, dispatcher,
                                BindingEngine::DispatchPolicy::AlwaysPost};

    Property<int> value{1};
    MemoryView direct_view;
    MemoryView posted_view;

    direct_engine.bind_int(value, direct_view);
    posted_engine.bind_int(value, posted_view);

    std::cout << "   绑定后       : direct = " << direct_view.integer
              << ", posted = " << posted_view.integer << '\n';

    value = 42;
    std::cout << "   改值后立刻   : direct = " << direct_view.integer
              << ", posted = " << posted_view.integer
              << "   (AlwaysPost 还在队列里)\n";

    const std::size_t processed = dispatcher->pump();
    std::cout << "   pump " << processed << " 条后: posted = " << posted_view.integer << '\n';

    return 0;
}
