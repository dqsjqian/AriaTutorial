// 教程用的内存适配器 —— 把 IViewAdapter 实现成一个"看不见的界面"。
//
// 它不做任何绘制, 只把值存在结构体里, 方便在命令行里观察绑定行为。
// 真实平台的适配器做的是同一件事, 只是把 set_text 换成 QLabel::setText /
// UILabel.text / DOM.textContent。第 17 章会逐行讲这个实现。
#pragma once

#include "aria/binding/view_adapter.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tutorial {

/// 极简信号: 够 adapter 用就行。断开时把槽置空, 已断开的槽不会被回调。
template <typename... Args>
class Signal {
public:
    aria::Subscription connect(std::function<void(Args...)> fn) {
        auto slot = std::make_shared<std::function<void(Args...)>>(std::move(fn));
        slots_.push_back(slot);
        return aria::Subscription{[slot] { *slot = nullptr; }};
    }

    void emit(const Args&... args) const {
        for (const auto& slot : slots_) {
            if (slot && *slot) {
                (*slot)(args...);
            }
        }
    }

private:
    std::vector<std::shared_ptr<std::function<void(Args...)>>> slots_;
};

/// 一个假的"控件"。真实平台里它是 QLabel / UILabel / DOM 节点。
struct MemoryView : public aria::binding::IView {
    std::string   text;
    bool          flag    = false;
    int           integer = 0;
    std::int64_t  i64     = 0;
    std::uint64_t u64     = 0;
    float         f32     = 0.0f;
    double        number  = 0.0;
    bool          visible = true;
    bool          enabled = true;

    Signal<std::string_view> text_changed;    // 用户输入
    Signal<bool>             bool_changed;    // 用户勾选
    Signal<int>              int_changed;
    Signal<double>           double_changed;
    Signal<>                 clicked;         // 用户点击

    [[nodiscard]] std::string_view kind() const noexcept override { return "memory"; }

    // 下面几个方法模拟"用户操作"。真实平台里由控件事件驱动。
    void user_type(std::string value) {
        text = std::move(value);
        text_changed.emit(text);
    }
    void user_toggle(bool value) {
        flag = value;
        bool_changed.emit(value);
    }
    void user_click() { clicked.emit(); }
};

/// 把 MemoryView 接到 BindingEngine 上的适配器。
class MemoryAdapter : public aria::binding::IViewAdapter {
public:
    [[nodiscard]] std::string_view platform_name() const noexcept override {
        return "Memory";
    }

    void set_text(aria::binding::IView& v, std::string_view t) override {
        static_cast<MemoryView&>(v).text = std::string(t);
    }
    std::string get_text(aria::binding::IView& v) override {
        return static_cast<MemoryView&>(v).text;
    }
    aria::Subscription on_text_changed(
        aria::binding::IView& v,
        std::function<void(std::string_view)> cb) override {
        return static_cast<MemoryView&>(v).text_changed.connect(std::move(cb));
    }

    void set_bool(aria::binding::IView& v, bool b) override {
        static_cast<MemoryView&>(v).flag = b;
    }
    bool get_bool(aria::binding::IView& v) override {
        return static_cast<MemoryView&>(v).flag;
    }
    aria::Subscription on_bool_changed(aria::binding::IView& v,
                                       std::function<void(bool)> cb) override {
        return static_cast<MemoryView&>(v).bool_changed.connect(std::move(cb));
    }

    void set_int(aria::binding::IView& v, int n) override {
        static_cast<MemoryView&>(v).integer = n;
    }
    int get_int(aria::binding::IView& v) override {
        return static_cast<MemoryView&>(v).integer;
    }
    aria::Subscription on_int_changed(aria::binding::IView& v,
                                      std::function<void(int)> cb) override {
        return static_cast<MemoryView&>(v).int_changed.connect(std::move(cb));
    }

    void set_int64(aria::binding::IView& v, std::int64_t n) override {
        static_cast<MemoryView&>(v).i64 = n;
    }
    std::int64_t get_int64(aria::binding::IView& v) override {
        return static_cast<MemoryView&>(v).i64;
    }
    aria::Subscription on_int64_changed(aria::binding::IView& v,
                                        std::function<void(std::int64_t)> cb) override {
        return aria::Subscription{};   // 本例不演示 int64 的双向
    }

    void set_uint64(aria::binding::IView& v, std::uint64_t n) override {
        static_cast<MemoryView&>(v).u64 = n;
    }
    std::uint64_t get_uint64(aria::binding::IView& v) override {
        return static_cast<MemoryView&>(v).u64;
    }
    aria::Subscription on_uint64_changed(aria::binding::IView& v,
                                         std::function<void(std::uint64_t)> cb) override {
        return aria::Subscription{};
    }

    void set_float(aria::binding::IView& v, float f) override {
        static_cast<MemoryView&>(v).f32 = f;
    }
    float get_float(aria::binding::IView& v) override {
        return static_cast<MemoryView&>(v).f32;
    }
    aria::Subscription on_float_changed(aria::binding::IView& v,
                                        std::function<void(float)> cb) override {
        return aria::Subscription{};
    }

    void set_double(aria::binding::IView& v, double d) override {
        static_cast<MemoryView&>(v).number = d;
    }
    double get_double(aria::binding::IView& v) override {
        return static_cast<MemoryView&>(v).number;
    }
    aria::Subscription on_double_changed(aria::binding::IView& v,
                                         std::function<void(double)> cb) override {
        return static_cast<MemoryView&>(v).double_changed.connect(std::move(cb));
    }

    void set_visible(aria::binding::IView& v, bool b) override {
        static_cast<MemoryView&>(v).visible = b;
    }
    void set_enabled(aria::binding::IView& v, bool b) override {
        static_cast<MemoryView&>(v).enabled = b;
    }

    aria::Subscription on_click(aria::binding::IView& v,
                                std::function<void()> cb) override {
        return static_cast<MemoryView&>(v).clicked.connect(std::move(cb));
    }
};

}  // namespace tutorial
