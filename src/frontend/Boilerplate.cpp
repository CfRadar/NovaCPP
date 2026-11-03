/* ============================================================================
   NOVACPP STARTER BOILERPLATE
   LIFETIME SAFETY: NovaCPP stores pointers to components for surgical updates.
   Ensure components have stable lifetime: static, global, or std::shared_ptr.
   NEVER instantiate components as local stack variables in route functions!
   ============================================================================ */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "../../novacpp/html.hpp"
#include "../../novacpp/state.hpp"
#include "../../novacpp/fetch.hpp"
#include "../../novacpp/graph.hpp"
#include "../backend/Database.hpp"
#include "../backend/Auth.hpp"

// 1. Reactive State Hooks (Session-Isolated)
static np::State<int> g_counter(0);
static np::State<std::string> g_lastSubmission("No feedback submitted yet.");

// 2. Modular UI Components
class HeaderComponent : public np::Component {
public:
    HeaderComponent() : Component("header-comp") {}
    void render(np::RenderContext& np) override {
        np << R"html(
        <div class="bento-card col-12" style="display:flex; justify-content:space-between; align-items:center;">
            <h1 style="margin:0; font-size:1.6rem;">NovaCPP Starter Application</h1>
            <button type="button" class="theme-toggle-btn" onclick="window.toggleNovaTheme()">Toggle Theme</button>
        </div>)html";
    }
};

class CounterCard : public np::Component {
public:
    CounterCard() : Component("counter-card") {}
    void render(np::RenderContext& np) override {
        np << R"html(<div class="bento-card col-6">
            <h3>Surgical Counter</h3>
            <p>Count: <strong style="font-size:1.3rem;">)html" + std::to_string(g_counter.get()) + R"html(</strong></p>
            <div style="display:flex; gap:0.5rem;">
                <button class="btn-secondary" nova-click="dec" nova-target="counter-card">&minus; Decrement</button>
                <button class="btn-primary" nova-click="inc" nova-target="counter-card">&plus; Increment</button>
            </div>
        </div>)html";
    }
};

class FormCard : public np::Component {
public:
    FormCard() : Component("form-card") {}
    void render(np::RenderContext& np) override {
        np << R"html(<div class="bento-card col-6">
            <h3>Reactive Form</h3>
            <form nova-form="feedback_form" nova-target="form-card" style="display:flex; gap:0.5rem; flex-direction:column;">
                <input type="text" name="user_feedback" placeholder="Enter note..." class="input-control" required />
                <button type="submit" class="btn-primary">Submit to Server</button>
            </form>
            <div style="margin-top:0.5rem; font-size:0.85rem;"><strong>Status:</strong> )html" + g_lastSubmission.get() + R"html(</div>
        </div>)html";
    }
};

class DatabaseCard : public np::Component {
public:
    DatabaseCard() : Component("db-card") {}
    void render(np::RenderContext& np) override {
        std::string rows = "0";
        try { rows = Database::queryScalar("SELECT COUNT(*) FROM users", "0"); } catch (...) {}
        np << R"html(<div class="bento-card col-6">
            <h3>SQLite WAL Engine</h3>
            <p>Registered users: <strong>)html" + rows + R"html(</strong></p>
            <button class="btn-secondary" nova-click="refresh_db" nova-target="db-card">Refresh Stats</button>
        </div>)html";
    }
};

class AgentCard : public np::Component {
private:
    std::string m_verdict{"Agent idle. Click Run to evaluate."};
public:
    AgentCard() : Component("agent-card") {}
    void setVerdict(std::string v) { m_verdict = std::move(v); }
    void render(np::RenderContext& np) override {
        np << R"html(<div class="bento-card col-6">
            <h3>NovaGraph Agentic AI</h3>
            <p style="font-size:0.85rem; font-family:monospace; background:rgba(0,0,0,0.04); padding:0.5rem;">)html" + m_verdict + R"html(</p>
            <button class="btn-primary" nova-click="run_agent" nova-target="agent-card">Execute Graph Step</button>
        </div>)html";
    }
};

// 3. Static Component Instances (Safe Lifetime)
static HeaderComponent g_headerComp;
static CounterCard g_counterCard;
static FormCard g_formCard;
static DatabaseCard g_dbCard;
static AgentCard g_agentCard;

// 4. Route Handlers
void renderHomePage(np::RenderContext& np) {
    np.onClick("inc", []() { g_counter = g_counter + 1; });
    np.onClick("dec", []() { g_counter = g_counter - 1; });
    np.onClick("refresh_db", []() {});
    np.onClick("run_agent", []() {
        struct AgentState { int step{0}; std::string msg; };
        np::Graph<AgentState> graph;
        graph.addNode("think", [](AgentState s) { s.step++; s.msg = "Evaluated at step " + std::to_string(s.step) + " -> Optimal."; return s; });
        graph.setEntryPoint("think");
        graph.addEdge("think", np::END);
        auto res = graph.compile().invoke(AgentState{});
        g_agentCard.setVerdict(res.msg);
    });
    np.onForm("feedback_form", [](const std::map<std::string, std::string>& data) {
        if (data.count("user_feedback")) g_lastSubmission = data.at("user_feedback");
    });

    np << "<div class='bento-grid'>";
    np.renderComponent(g_headerComp);
    np.renderComponent(g_counterCard);
    np.renderComponent(g_formCard);
    np.renderComponent(g_dbCard, 3000);
    np.renderComponent(g_agentCard);
    np << "</div>";
}

void renderAboutPage(np::RenderContext& np) {
    np << "<div class='bento-grid'><div class='bento-card col-12'><h1>About</h1><p>Single Page Navigation powered by C++17.</p><a href='/' class='btn-primary' nova-link>&larr; Back to Home</a></div></div>";
}
