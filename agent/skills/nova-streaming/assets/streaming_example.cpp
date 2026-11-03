#include "novacpp/nova.hpp"
#include <iostream>

void demonstrateStreaming() {
    np::SSEChannel emitter;

    // 1. Subscribe HTTP wire listener
    emitter.subscribe([](const np::StreamMessage& msg) {
        std::cout << "[SSE PACKET] " << msg.eventName() << " (Length: " << msg.payload.size() << ")\n";
        std::cout << msg.toSSE();
    });

    // 2. Accumulate tokens in thread-safe buffer
    np::StreamBuffer buffer;

    // 3. Emit lifecycle events
    emitter.emitNodeStart("research_node");

    std::vector<std::string> tokens = {
        "NovaCPP ", "provides ", "sub-millisecond ", "Server-Sent ", "Events.\n"
    };

    for (const auto& tok : tokens) {
        buffer.append(tok);
        emitter.emitToken("research_node", tok);
    }

    emitter.emitNodeEnd("research_node", "Completed token stream");

    std::cout << "Accumulated buffer (" << buffer.tokenCount() << " tokens):\n" << buffer.str();
}

int main() {
    demonstrateStreaming();
    return 0;
}
