// ch16: 自己写一个 IViewAdapter -- 从 25 个纯虚方法到 5 个
//
// IViewAdapter 是个"宽接口": text / bool / int / int64 / uint64 / float /
// double 各三个方法, 再加 visible / enabled / click / platform_name, 一共
// 25 个纯虚函数。新写一个适配器不该被逼着全部实现。
//
// ViewAdapterBase 把每个操作都实现成契约要求的"不支持"路径 (告警一次 +
// 返回安全默认值), 作者只重写平台真正支持的那几个。
#include "aria/aria.hpp"
#include "aria/binding/binding_engine.hpp"
#include "aria/binding/view_adapter_base.hpp"

#include "common/memory_adapter.hpp"   // 复用其中的教程用 Signal

#include <iostream>
#include <memory>
#include <string>

using namespace aria;
using namespace aria::binding;

namespace {

/// 一个只有"文本"和"点击"两种能力的极简控件。
struct LiteView : public IView {
    std::string text;

    tutorial::Signal<std::string_view> text_signal;
    tutorial::Signal<>                 click_signal;

    [[nodiscard]] std::string_view kind() const noexcept override { return "lite"; }

    void user_type(std::string value) {
        text = std::move(value);
        text_signal.emit(text);
    }
    void user_click() { click_signal.emit(); }
};

/// 只重写 5 个方法的适配器。其余 20 个继承自 ViewAdapterBase。
class LiteAdapter final : public ViewAdapterBase {
public:
    [[nodiscard]] std::string_view platform_name() const noexcept override {
        return "Lite";          // 这个没有合理默认值, 必须重写
    }

    void set_text(IView& v, std::string_view t) override {
        static_cast<LiteView&>(v).text = std::string(t);
    }
    std::string get_text(IView& v) override {
        return static_cast<LiteView&>(v).text;
    }
    Subscription on_text_changed(IView& v,
                                 std::function<void(std::string_view)> cb) override {
        return static_cast<LiteView&>(v).text_signal.connect(std::move(cb));
    }
    Subscription on_click(IView& v, std::function<void()> cb) override {
        return static_cast<LiteView&>(v).click_signal.connect(std::move(cb));
    }
};

}  // namespace

int main() {
    auto adapter = std::make_shared<LiteAdapter>();
    BindingEngine engine{adapter};

    std::cout << "平台 = " << adapter->platform_name() << '\n';

    std::cout << "\n== 1. 重写过的能力: 文本双向 ==\n";
    Property<std::string> keyword{"aria"};
    LiteView search_box;

    engine.bind_text(keyword, search_box);

    std::cout << "   绑定后 view.text = \"" << search_box.text << "\"\n";
    search_box.user_type("mvvm");
    std::cout << "   用户输入 -> VM = \"" << keyword.get() << "\"\n";

    std::cout << "\n== 2. 重写过的能力: 点击 -> Command ==\n";
    int clicks = 0;
    Command<> go([&] { ++clicks; });
    LiteView go_button;

    engine.bind_command(go, go_button);

    go_button.user_click();
    std::cout << "   点一次   -> clicks = " << clicks << '\n';
    go_button.user_click();
    std::cout << "   再点一次 -> clicks = " << clicks << '\n';

    std::cout << "\n== 3. 没重写的能力: 走安全的降级路径 ==\n";
    Property<int> page{1};
    LiteView page_box;

    // LiteView 没有整型通道。ViewAdapterBase 会告警, set 变空操作,
    // get 返回 0 -- 不会崩, 也不会静默写坏数据。
    engine.bind_int(page, page_box);
    page = 2;

    std::cout << "   page 实际值      = " << page.get() << '\n';
    std::cout << "   adapter 读回控件 = " << adapter->get_int(page_box) << '\n';
    std::cout << "   (整型通道未实现, 读回 0)\n";

    std::cout << "\n== 4. 契约自检 ==\n";
    std::cout << "   框架自带一致性测试套件, 第三方适配器可以拿它自查:\n";
    std::cout << "   aria/binding/testing/adapter_conformance.hpp\n";

    return 0;
}
