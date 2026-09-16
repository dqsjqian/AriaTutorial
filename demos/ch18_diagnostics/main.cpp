// ch18: 诊断 -- 把响应式图导出来看
#include "aria/aria.hpp"
#include "aria/reactive/inspector.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace aria;
using aria::reactive::GraphInspector;
using aria::reactive::Node;

int main() {
    std::cout << "== 1. 先给节点起名字 ==\n";
    Property<double> price{100.0};
    Property<int>    quantity{2};
    Property<double> discount{0.9};

    price.set_debug_name("price");
    quantity.set_debug_name("quantity");
    discount.set_debug_name("discount");

    Computed<double> subtotal{[&] { return price.get() * quantity.get(); }};
    subtotal.set_debug_name("subtotal");

    Computed<double> total{[&] { return subtotal.get() * discount.get(); }};
    total.set_debug_name("total");

    std::cout << "   total = " << total.get() << '\n';

    std::cout << "\n== 2. to_text: 人类可读的依赖图 ==\n";
    std::cout << GraphInspector::to_text({static_cast<const Node*>(&total)});

    std::cout << "\n== 3. to_dot: 粘进 Graphviz 就能画图 ==\n";
    std::cout << GraphInspector::to_dot({static_cast<const Node*>(&total)}, "invoice");

    std::cout << "\n== 4. to_json: 喂给工具链 ==\n";
    std::cout << GraphInspector::to_json({static_cast<const Node*>(&total)}) << '\n';

    std::cout << "\n== 5. TraceSink: 捕获事件流 ==\n";
    std::vector<TraceEvent> events;
    {
        ScopedTraceSink guard{[&events](const TraceEvent& ev) { events.push_back(ev); }};
        price = 200.0;
        quantity = 3;
    }

    std::cout << "   两次写入共捕获 " << events.size() << " 条事件\n";
    for (const auto& ev : events) {
        std::cout << "   category = " << ev.category_name() << '\n';
    }

    std::cout << "\n== 6. 没起名字的节点会回退成 <Kind>#<id> ==\n";
    Property<int> anonymous{0};
    Computed<int> also_anonymous{[&] { return anonymous.get() + 1; }};
    (void)also_anonymous.get();
    std::cout << GraphInspector::to_text({static_cast<const Node*>(&also_anonymous)});

    return 0;
}
