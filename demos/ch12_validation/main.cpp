// ch12: Validator -- 校验也是响应式的
//
// 两个对外类型的区别:
//   ValidationResult -- 只有 valid + errors, 轻量
//   ValidationState  -- 另外带 pending / touched / dirty / warnings
// 需要显示警告或异步状态时, 用 state()。
#include "aria/aria.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace aria;

int main() {
    std::cout << "== 1. must 产生 error, should 产生 warning ==\n";
    Property<std::string>  name{""};
    Validator<std::string> name_v{name, "user.name"};

    name_v.must([](const std::string& s) { return !s.empty(); }, "不能为空", "not_empty")
          .must([](const std::string& s) { return s.size() <= 8; }, "最多 8 个字符", "max_len")
          .should([](const std::string& s) { return s != "admin"; }, "不建议使用 admin", "avoid_admin");

    auto print_name = [&] {
        const auto& st = name_v.state().get();
        std::cout << "   值 = \"" << name.get() << "\""
                  << ", 错误 " << st.errors.size()
                  << " 条, 警告 " << st.warnings.size() << " 条";
        if (!st.errors.empty()) {
            std::cout << "  首个错误: " << st.errors[0].message;
        }
        std::cout << '\n';
    };

    print_name();
    auto name_sub = name_v.state().on_changed([&](const auto&) { print_name(); });

    std::cout << "   name = \"admin\"\n";
    name = "admin";
    std::cout << "   name = \"admintest\"\n";
    name = "admintest";
    std::cout << "   name = \"aria\"\n";
    name = "aria";

    std::cout << "\n== 2. 按规则 id 精确查询 ==\n";
    const auto& st = name_v.state().get();
    std::cout << "   has_error_with_rule(\"not_empty\") = "
              << (st.has_error_with_rule("not_empty") ? "true" : "false") << '\n';
    std::cout << "   has_error_with_rule(\"max_len\") = "
              << (st.has_error_with_rule("max_len") ? "true" : "false") << '\n';

    std::cout << "\n== 3. 异步校验: pending 状态 ==\n";
    Property<std::string>  user{"aria"};
    Validator<std::string> user_v{user, "user.id"};

    auto print_pending = [&] {
        const auto& s = user_v.state().get();
        std::cout << "   pending = " << (s.pending ? "true" : "false")
                  << ", 错误 " << s.errors.size() << " 条\n";
    };

    print_pending();
    std::cout << "   发起异步校验 (去服务端查重) ...\n";
    user_v.begin_pending();
    print_pending();
    std::cout << "   服务端返回: 已被占用\n";
    user_v.end_pending(std::vector<std::string>{"该用户名已被占用"});
    print_pending();
    std::cout << "   重新查一次, 这次返回可用\n";
    user_v.begin_pending();
    user_v.end_pending();
    print_pending();

    std::cout << "\n== 4. 跨字段: 确认密码要和密码一致 ==\n";
    Property<std::string> password{"aria2026"};
    Property<std::string> confirm{"aria2027"};

    Validator<std::string> confirm_v{confirm, "form.confirm"};
    confirm_v.must([&](const std::string& s) { return s == password.get(); },
                   "两次输入的密码不一致", "match");

    auto print_confirm = [&] {
        const auto& s = confirm_v.state().get();
        std::cout << "   password = \"" << password.get()
                  << "\", confirm = \"" << confirm.get() << "\"  ->  ";
        if (s.errors.empty()) {
            std::cout << "通过";
        } else {
            std::cout << s.errors[0].message;
        }
        std::cout << '\n';
    };

    print_confirm();
    auto confirm_sub = confirm_v.state().on_changed([&](const auto&) { print_confirm(); });

    std::cout << "   confirm 改成 \"aria2026\"\n";
    confirm = "aria2026";

    return 0;
}
