// ch15: Qt6 适配器 -- 界面照旧用 Designer 拖, C++ 只负责接线
//
// 本示例需要 Qt6, 默认不参与构建。开启方式:
//
//   cmake -S . -B build -DARIA_ROOT=<Aria 源码树> \
//         -DARIA_TUTORIAL_QT6=ON \
//         -DCMAKE_PREFIX_PATH=<Qt6 安装路径>
//   cmake --build build
//
// 界面部分就是普通的 Qt 控件, 和 Aria 没有任何关系。Aria 只做一件事:
// 把已经存在的控件交给 BindingEngine。
#include "aria/adapters/qt6/qt_adapter.hpp"
#include "aria/aria.hpp"
#include "aria/binding/binding_engine.hpp"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <iostream>
#include <memory>
#include <string>

using namespace aria;
using namespace aria::adapters::qt6;

// ---------------------------------------------------------------------------
// 业务逻辑: 纯 C++, 不认识 Qt
// ---------------------------------------------------------------------------
struct TipViewModel {
    Property<double> bill{200.0};    // 账单总额
    Property<int>    people{2};      // 分摊人数

    Computed<double> per_person{[&] { return bill.get() / people.get(); }};
    Computed<bool>   can_settle{[&] { return people.get() > 0; }};   // 除零保护
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // ---- 上半: 界面。和写普通 Qt 程序完全一样 ----
    QWidget window;
    auto* layout     = new QVBoxLayout{&window};
    auto* title      = new QLabel{QString::fromUtf8("AA 制摊分"), &window};
    auto* per_person = new QLabel{&window};
    auto* settle     = new QPushButton{QString::fromUtf8("摊分"), &window};

    layout->addWidget(title);
    layout->addWidget(per_person);
    layout->addWidget(settle);
    window.resize(320, 160);

    // ---- 下半: 接线。三步, 每个平台都一样 ----
    // 1) 造平台适配器
    auto adapter = std::make_shared<QtAdapter>();
    // 2) 用它造 BindingEngine
    binding::BindingEngine engine{adapter};

    TipViewModel vm;
    int settles = 0;
    Command<> do_settle{[&] { ++settles; }};

    // 3) 把控件和 Property 绑上
    engine.bind_text_projected(vm.per_person, adapter->view_for(per_person),
        [](double v) {
            return QString::fromUtf8("每人付: ¥ %1").arg(v, 0, 'f', 2).toStdString();
        });
    engine.bind_enabled(vm.can_settle, adapter->view_for(settle));
    engine.bind_command(do_settle, adapter->view_for(settle));

    // ---- 之后只改数据, 界面自己跟着变 ----
    vm.people = 4;      // 标签 -> 每人付: ¥ 50.00
    vm.bill   = 90.0;   // 标签 -> 每人付: ¥ 22.50

    std::cout << "窗口已显示。当前每人应付 "
              << vm.per_person.get() << " 元。关掉窗口结束程序。\n";

    window.show();
    return app.exec();
}
