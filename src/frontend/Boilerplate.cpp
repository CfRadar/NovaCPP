/* ============================================================================
   NOVACPP STARTER BOILERPLATE
   ============================================================================
   IMPORTANT LIFETIME SAFETY RULE:
   NovaCPP stores pointers to components for surgical DOM patch updates.
   Always ensure components have a stable lifetime:
     1. Global or namespace scope (as below).
     2. Static variables inside functions: static MyComponent myComp;
     3. Shared pointers on the heap: np.renderComponent(std::make_shared<MyComponent>());
   NEVER instantiate components as local stack variables in route functions!
   ============================================================================ */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

// Core NovaCPP Headers
#include "../../novacpp/html.hpp"        // HTML builder, surgical components & server
#include "../../novacpp/state.hpp"       // Reactive session-isolated state hooks
#include "../../novacpp/fetch.hpp"       // Outbound REST client
#include "../../novacpp/graph.hpp"       // NovaGraph Pregel Agentic AI engine
#include "../../novacpp/simd_vector.hpp" // In-process SIMD vector memory
#include "../backend/Database.hpp"       // SQLite WAL connection pool
#include "../backend/Auth.hpp"           // User authentication and session management

// ============================================================================
// 1. REACTIVE STATE HOOKS (Thread-Safe, Session-Isolated)
// ============================================================================
static np::State<int> g_counter(0);
static np::State<std::string> g_lastSubmission("No feedback submitted yet.");

// ============================================================================
// 2. MODULAR UI COMPONENTS
// ============================================================================

// Component 1: Interactive Counter (Surgical DOM patching)
class CounterCard : public np::Component {
public:
    CounterCard() : Component("counter-card") {}

    void render(np::RenderContext& np) override {
        np << R"html(
        <div class="bento-card col-6">
            <div style="display:flex; justify-content:space-between; align-items:center;">
                <h3 style="margin:0;">Surgical Counter</h3>
                <span class="badge">DOM Morph</span>
            </div>
            <p style="margin-top:0.75rem;">Current value: <strong style="font-size:1.4rem;">)html" 
               + std::to_string(g_counter.get()) + R"html(</strong></p>
            <div class="btn-group" style="display:flex; gap:0.5rem;">
                <button class="btn-secondary" nova-click="dec" nova-target="counter-card">&minus; Decrement</button>
                <button class="btn-primary" nova-click="inc" nova-target="counter-card">&plus; Increment</button>
            </div>
        </div>
        )html";
    }
};

// Component 2: Reactive Form Component (Client-to-Server dispatch)
class FormCard : public np::Component {
public:
    FormCard() : Component("form-card") {}

    void render(np::RenderContext& np) override {
        np << R"html(
        <div class="bento-card col-6">
            <h3 style="margin:0 0 0.5rem 0;">Reactive Submission Form</h3>
            <form nova-form="feedback_form" nova-target="form-card" style="display:flex; flex-direction:column; gap:0.5rem;">
                <input type="text" name="user_feedback" placeholder="Enter status note..." class="input-control" required />
                <button type="submit" class="btn-primary">Submit to Server</button>
            </form>
            <div style="margin-top:0.75rem; padding:0.5rem; background:rgba(0,0,0,0.05); border-radius:6px; font-size:0.85rem;">
                <strong>Status:</strong> )html" + g_lastSubmission.get() + R"html(
            </div>
        </div>
        )html";
    }
};

// Component 3: Database & Telemetry Monitor (Live polling every 3000ms)
class TelemetryCard : public np::Component {
public:
    TelemetryCard() : Component("telemetry-card") {}

    void render(np::RenderContext& np) override {
        std::string totalRows = "0";
        try {
            totalRows = Database::queryScalar("SELECT COUNT(*) FROM users", "0");
        } catch (...) {}

        np << R"html(
        <div class="bento-card col-6">
            <div style="display:flex; justify-content:space-between; align-items:center;">
                <h3 style="margin:0;">SQLite WAL Engine</h3>
                <span class="badge badge-success">Active</span>
            </div>
            <p>Database queries executed on zero-overhead thread pool.</p>
            <p>Registered users: <strong>)html" + totalRows + R"html(</strong></p>
            <button class="btn-secondary" nova-click="refresh_db" nova-target="telemetry-card">Refresh Stats</button>
        </div>
        )html";
    }
};

// Component 4: Agentic AI Workflow Card
class AgentCard : public np::Component {
private:
    std::string m_lastVerdict{"Agent idle. Click Run to evaluate."};

public:
    AgentCard() : Component("agent-card") {}

    void setVerdict(std::string verdict) {
        m_lastVerdict = std::move(verdict);
    }

    void render(np::RenderContext& np) override {
        np << R"html(
        <div class="bento-card col-6">
            <div style="display:flex; justify-content:space-between; align-items:center;">
                <h3 style="margin:0;">NovaGraph Agentic AI</h3>
                <span class="badge badge-info">Pregel Engine</span>
            </div>
            <p style="font-size:0.85rem; color:#888;">Simulates in-process cyclic agent reasoning.</p>
            <div style="padding:0.75rem; background:rgba(0,0,0,0.04); border-radius:6px; font-family:monospace; font-size:0.85rem; margin-bottom:0.75rem;">
                )html" + m_lastVerdict + R"html(
            </div>
            <button class="btn-primary" nova-click="run_agent" nova-target="agent-card">Execute Graph Step</button>
        </div>
        )html";
    }
};

// ============================================================================
// 3. STATIC COMPONENT INSTANCES (Safe Lifetime)
// ============================================================================
static CounterCard g_counterCard;
static FormCard g_formCard;
static TelemetryCard g_telemetryCard;
static AgentCard g_agentCard;

// ============================================================================
// 4. ROUTE DEFINITIONS & DISPATCH HANDLERS
// ============================================================================
void renderHomePage(np::RenderContext& np) {
    // Action: Increment
    np.onClick("inc", []() {
        g_counter = g_counter + 1;
    });

    // Action: Decrement
    np.onClick("dec", []() {
        g_counter = g_counter - 1;
    });

    // Action: Database refresh
    np.onClick("refresh_db", []() {
        // Safe query trigger
    });

    // Action: Execute Agentic AI Graph
    np.onClick("run_agent", []() {
        // Inline lightweight Pregel workflow
        struct AgentState {
            int stepCount{0};
            std::string decision;
        };

        np::Graph<AgentState> graph;
        graph.addNode("think", [](AgentState s) {
            s.stepCount++;
            s.decision = "Evaluated environment at step " + std::to_string(s.stepCount) + " -> Optimal state.";
            return s;
        });
        graph.setEntryPoint("think");
        graph.addEdge("think", np::END);

        auto finalState = graph.compile().invoke(AgentState{});
        g_agentCard.setVerdict(finalState.decision);
    });

    // Form: Feedback submission
    np.onForm("feedback_form", [](const std::map<std::string, std::string>& data) {
        if (data.count("user_feedback")) {
            g_lastSubmission = data.at("user_feedback");
        }
    });

    // Page Layout: 12-Column Responsive Bento Grid
    np << R"html(
    <div class="bento-grid">
        <div class="bento-card col-12">
            <div style="display:flex; justify-content:space-between; align-items:center;">
                <div>
                    <h1 style="margin:0; font-size:1.75rem;">Welcome to NovaCPP</h1>
                    <p style="margin:0.25rem 0 0 0; color:#888;">Ultra-high performance C++17 full-stack web framework</p>
                </div>
                <button type="button" class="theme-toggle-btn" onclick="window.toggleNovaTheme()">Toggle Theme</button>
            </div>
        </div>
    )html";

    // Surgical component hydration
    np.renderComponent(g_counterCard);
    np.renderComponent(g_formCard);
    np.renderComponent(g_telemetryCard, 3000); // Polling every 3000ms
    np.renderComponent(g_agentCard);

    np << "</div>";
}

void renderAboutPage(np::RenderContext& np) {
    np << R"html(
    <div class="bento-grid">
        <div class="bento-card col-12">
            <h1>About Page</h1>
            <p>Single Page Navigation powered by C++17.</p>
            <a href="/" class="btn-primary" nova-link>&larr; Back to Home</a>
        </div>
    </div>
    )html";
}
