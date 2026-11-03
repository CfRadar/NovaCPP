#pragma once

#include <string>
#include <stdexcept>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace np {

// Detailed payload capturing Human-in-the-Loop review context and UI directives
struct InterruptPayload {
    std::string threadId;
    std::string message;
    std::string actionRequired = "approve";
    std::string nodeId;
    std::string dataJson = "{}";
    double amount = 0.0;
    bool resumeSameNode = false;

    InterruptPayload() = default;

    InterruptPayload(std::string msg, std::string action = "approve", std::string node = "", bool reRun = false)
        : message(std::move(msg)), actionRequired(std::move(action)), nodeId(std::move(node)), resumeSameNode(reRun) {}
};

// Exception thrown when an active graph execution pauses for human input or permission
class GraphInterruptException : public std::exception {
private:
    InterruptPayload m_payload;
    std::string m_whatMsg;

public:
    explicit GraphInterruptException(InterruptPayload payload)
        : m_payload(std::move(payload)) {
        m_whatMsg = "Graph execution interrupted at node '" + m_payload.nodeId +
                    "': " + m_payload.message + " (Action: " + m_payload.actionRequired + ")";
    }

    const InterruptPayload& payload() const noexcept {
        return m_payload;
    }

    InterruptPayload& payload() noexcept {
        return m_payload;
    }

    const char* what() const noexcept override {
        return m_whatMsg.c_str();
    }
};

// Explicit programmatic interruption invoked from within any node function
inline void interrupt(InterruptPayload payload) {
    throw GraphInterruptException(std::move(payload));
}

inline void interrupt(const std::string& message, const std::string& actionRequired = "approve", bool resumeSameNode = false) {
    InterruptPayload p;
    p.message = message;
    p.actionRequired = actionRequired;
    p.resumeSameNode = resumeSameNode;
    throw GraphInterruptException(std::move(p));
}

// Monolithic UI Component Generator for Human-in-the-Loop
class HITLUI {
public:
    // Generate a responsive Bento-style approval card with nova-form binding
    static std::string renderApprovalCard(
        const InterruptPayload& payload,
        const std::string& formAction = "/nova/form/approve_action",
        const std::string& containerId = "approval-card"
    ) {
        std::ostringstream ss;
        ss << "<div class='bento-card col-6' id='" << containerId << "'>\n"
           << "  <div class='card-header'>\n"
           << "    <span class='badge badge-warning'>Human Review Required</span>\n"
           << "    <h3>" << (payload.nodeId.empty() ? "Pending Decision" : payload.nodeId) << "</h3>\n"
           << "  </div>\n"
           << "  <div class='card-body'>\n"
           << "    <p class='interrupt-message'>" << payload.message << "</p>\n";
        
        if (payload.amount > 0.0) {
            ss << "    <div class='amount-highlight'>Amount: $" << payload.amount << "</div>\n";
        }

        ss << "  </div>\n"
           << "  <div class='card-footer'>\n"
           << "    <form nova-form action='" << formAction << "' method='POST'>\n"
           << "      <input type='hidden' name='thread_id' value='" << payload.threadId << "' />\n"
           << "      <input type='hidden' name='node_id' value='" << payload.nodeId << "' />\n"
           << "      <input type='hidden' name='action' value='" << payload.actionRequired << "' />\n"
           << "      <button type='submit' name='decision' value='approved' class='btn btn-primary'>Approve & Resume</button>\n"
           << "      <button type='submit' name='decision' value='rejected' class='btn btn-secondary'>Reject</button>\n"
           << "    </form>\n"
           << "  </div>\n"
           << "</div>\n";
        return ss.str();
    }

    // Generate a textual prompt card requiring user input (e.g. clarification)
    static std::string renderInputPrompt(
        const InterruptPayload& payload,
        const std::string& inputName = "user_input",
        const std::string& placeholder = "Enter your feedback or clarification...",
        const std::string& formAction = "/nova/form/submit_input",
        const std::string& containerId = "input-prompt-card"
    ) {
        std::ostringstream ss;
        ss << "<div class='bento-card col-6' id='" << containerId << "'>\n"
           << "  <div class='card-header'>\n"
           << "    <span class='badge badge-info'>Action Required</span>\n"
           << "    <h3>" << (payload.nodeId.empty() ? "User Input Required" : payload.nodeId) << "</h3>\n"
           << "  </div>\n"
           << "  <div class='card-body'>\n"
           << "    <p class='interrupt-message'>" << payload.message << "</p>\n"
           << "    <form nova-form action='" << formAction << "' method='POST'>\n"
           << "      <input type='hidden' name='thread_id' value='" << payload.threadId << "' />\n"
           << "      <input type='hidden' name='node_id' value='" << payload.nodeId << "' />\n"
           << "      <div class='form-group'>\n"
           << "        <textarea name='" << inputName << "' class='form-control' placeholder='" << placeholder << "' rows='3' required></textarea>\n"
           << "      </div>\n"
           << "      <button type='submit' class='btn btn-primary'>Submit Response</button>\n"
           << "    </form>\n"
           << "  </div>\n"
           << "</div>\n";
        return ss.str();
    }
};

} // namespace np

