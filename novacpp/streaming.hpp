#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <mutex>
#include <chrono>
#include <sstream>
#include <memory>

namespace np {

enum class StreamEventType {
    Token,
    NodeStart,
    NodeEnd,
    ToolStart,
    ToolEnd,
    StateSnapshot,
    Custom
};

struct StreamMessage {
    StreamEventType type = StreamEventType::Token;
    std::string nodeName;
    std::string payload;
    std::string customEvent;
    int64_t timestamp = 0;

    StreamMessage() {
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    StreamMessage(StreamEventType t, std::string node, std::string p, std::string custom = "")
        : type(t), nodeName(std::move(node)), payload(std::move(p)), customEvent(std::move(custom)) {
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    std::string eventName() const {
        switch (type) {
            case StreamEventType::Token: return "stream-token";
            case StreamEventType::NodeStart: return "node-start";
            case StreamEventType::NodeEnd: return "node-end";
            case StreamEventType::ToolStart: return "tool-start";
            case StreamEventType::ToolEnd: return "tool-end";
            case StreamEventType::StateSnapshot: return "state-update";
            case StreamEventType::Custom: return customEvent.empty() ? "custom" : customEvent;
        }
        return "message";
    }

    std::string toSSE() const {
        return "event: " + eventName() + "\ndata: " + payload + "\n\n";
    }
};

// Accumulator buffer for streaming tokens
class StreamBuffer {
private:
    std::string m_buffer;
    size_t m_tokenCount = 0;
    mutable std::mutex m_mtx;

public:
    StreamBuffer() = default;

    void append(std::string_view token) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_buffer.append(token.data(), token.size());
        m_tokenCount++;
    }

    std::string str() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_buffer;
    }

    size_t tokenCount() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_tokenCount;
    }

    size_t byteSize() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_buffer.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_buffer.clear();
        m_tokenCount = 0;
    }
};

// Stream Emitter for Server-Sent Events, token streams, and graph lifecycle observability
class StreamEmitter {
public:
    using Listener = std::function<void(const StreamMessage&)>;
    using Broadcaster = std::function<void(const std::string&, const std::string&)>;

private:
    Broadcaster m_broadcaster;
    std::vector<Listener> m_subscribers;
    mutable std::mutex m_mtx;

public:
    StreamEmitter() = default;

    template <typename TBuilder>
    explicit StreamEmitter(TBuilder* builder) {
        if (builder) {
            m_broadcaster = [builder](const std::string& ev, const std::string& d) {
                builder->broadcastHMR(ev, d);
            };
        }
    }

    explicit StreamEmitter(Broadcaster broadcaster)
        : m_broadcaster(std::move(broadcaster)) {}

    template <typename TBuilder>
    void setBuilder(TBuilder* builder) {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (builder) {
            m_broadcaster = [builder](const std::string& ev, const std::string& d) {
                builder->broadcastHMR(ev, d);
            };
        } else {
            m_broadcaster = nullptr;
        }
    }

    void setBroadcaster(Broadcaster broadcaster) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_broadcaster = std::move(broadcaster);
    }

    void subscribe(Listener listener) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_subscribers.push_back(std::move(listener));
    }

    void clearSubscribers() {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_subscribers.clear();
    }

    size_t subscriberCount() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_subscribers.size();
    }

    void emit(const StreamMessage& msg) {
        std::vector<Listener> subsCopy;
        Broadcaster broadcasterCopy;
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            subsCopy = m_subscribers;
            broadcasterCopy = m_broadcaster;
        }

        if (broadcasterCopy) {
            broadcasterCopy(msg.eventName(), msg.payload);
        }

        for (auto& sub : subsCopy) {
            if (sub) sub(msg);
        }
    }

    void emitToken(std::string_view token) {
        emit(StreamMessage(StreamEventType::Token, "agent-live-tokens", std::string(token)));
    }

    void emitNodeStart(const std::string& nodeName) {
        emit(StreamMessage(StreamEventType::NodeStart, nodeName, nodeName));
    }

    void emitNodeEnd(const std::string& nodeName, const std::string& summary = "") {
        emit(StreamMessage(StreamEventType::NodeEnd, nodeName, summary.empty() ? nodeName : summary));
    }

    void emitToolProgress(const std::string& toolName, const std::string& statusHtml) {
        emit(StreamMessage(StreamEventType::ToolStart, toolName, statusHtml));
    }

    void emitToolResult(const std::string& toolName, const std::string& resultJson) {
        emit(StreamMessage(StreamEventType::ToolEnd, toolName, resultJson));
    }

    void emitStateSnapshot(const std::string& stateJson) {
        emit(StreamMessage(StreamEventType::StateSnapshot, "graph", stateJson));
    }

    void emitCustom(const std::string& eventName, const std::string& data) {
        emit(StreamMessage(StreamEventType::Custom, "", data, eventName));
    }

    void emitStreamSwap(const std::string& targetId, const std::string& html) {
        std::string jsonPayload = "{\"target\":\"" + targetId + "\",\"html\":\"" + html + "\"}";
        emit(StreamMessage(StreamEventType::Custom, targetId, jsonPayload, "stream-swap"));
    }
};

} // namespace np
