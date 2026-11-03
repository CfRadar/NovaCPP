#include "novacpp/nova.hpp"
#include <iostream>

void demonstrateOutboundFetch() {
    // 1. GET request
    np::FetchResponse res = np::get("https://jsonplaceholder.typicode.com/todos/1", {
        {"Accept", "application/json"}
    });

    if (res.isOk()) {
        std::string id = res.jsonField("id");
        std::string title = res.jsonField("title");
        std::cout << "Todo #" << id << ": " << title << std::endl;
    }

    // 2. POST request
    std::string postPayload = R"({"title":"NovaCPP Task","completed":false,"userId":1})";
    np::FetchResponse postRes = np::post(
        "https://jsonplaceholder.typicode.com/todos",
        postPayload,
        {{"Content-Type", "application/json"}}
    );

    if (postRes.status == 201) {
        std::cout << "Created task: " << postRes.jsonField("id") << std::endl;
    }
}

int main() {
    demonstrateOutboundFetch();
    return 0;
}
