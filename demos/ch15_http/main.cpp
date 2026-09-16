// ch15: HTTP 适配器 -- 把浏览器当界面
//
// 本示例需要启用 Aria 的 HTTP 适配器, 默认不参与构建:
//
//   cmake -S . -B build -DARIA_ROOT=<Aria 源码树> -DARIA_TUTORIAL_HTTP=ON
//   cmake --build build
//
// 这是 Aria 最特别的一个适配器: 浏览器里没有 C++, 前端就是普通 HTML/JS,
// C++ 跑在服务端。Property 的变化经 SSE 推给浏览器, 用户操作经 REST 回来。
#include "aria/adapters/http/http_adapter.hpp"
#include "aria/aria.hpp"
#include "aria/binding/binding_engine.hpp"
#include "aria/runtime/dispatcher.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace aria;
using namespace aria::adapters::http;

int main() {
    HttpAdapterConfig config;
    config.port           = 9090;   // 0 表示让系统分配一个空闲端口
    config.worker_threads = 4;

    auto http       = std::make_shared<HttpAdapter>(config);
    auto dispatcher = std::make_shared<runtime::SimpleDispatcher>();

    // ---- 业务逻辑: 依然是纯 C++ ----
    Property<std::string> message{"你好, Aria"};

    // ---- 接线 ----
    binding::BindingEngine engine{http, dispatcher,
        binding::BindingEngine::DispatchPolicy::SmartMarshal};

    // 浏览器里的"控件"在这里是字符串 ID, 对应前端页面上 id="title" 的元素
    auto& title = http->register_view("title", "text");
    engine.bind_text(message, title);

    if (!http->start()) {
        std::cerr << "服务启动失败\n";
        return 1;
    }

    std::cout << "服务已启动。\n";
    std::cout << "在浏览器打开: http://127.0.0.1:" << http->actual_port() << "\n";
    std::cout << "\n";
    std::cout << "在下面输入新内容并回车, 页面会立刻更新 (走 SSE 推送):\n";

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            break;
        }
        message = line;                       // 只改数据, 推送由框架完成
        dispatcher->pump();                   // 宿主负责在 graph 线程上泵消息
        std::cout << "已推送: " << message.get() << '\n';
    }

    http->stop();
    std::cout << "服务已停止。\n";
    return 0;
}
