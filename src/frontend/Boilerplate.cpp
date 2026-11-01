/* ============================================================================
   NOVACPP STARTER BOILERPLATE
   Use this minimal template to start coding your own web application.
   You can copy the code below or rename this file to App.cpp.
   ============================================================================ */

#include <string>
#include <vector>
#include <map>
#include <iostream>

// All Framework Headers
#include "../../novacpp/html.hpp"        // Core HTML builder, components & server
#include "../../novacpp/state.hpp"       // Reactive session-isolated state hooks
#include "../../novacpp/fetch.hpp"       // Native outbound REST client
#include "../../novacpp/worker_pool.hpp" // In-process background worker thread pool
#include "../../novacpp/env.hpp"         // Zero-dependency .env loader
#include "../backend/Database.hpp"       // SQLite WAL connection pool & query engine
#include "../backend/Auth.hpp"           // User authentication & session management

// ============================================================================
// 1. REACTIVE STATE HOOKS
// Thread-safe variables automatically isolated per user session.
// ============================================================================
np::State<int> counter(0);
np::State<std::string> message("Welcome to your new NovaCPP application!");

// ============================================================================
// 2. MODULAR UI COMPONENTS
// Inherit from np::Component, implement render(), and target by component ID.
// ============================================================================
class HeaderComponent : public np::Component {
public:
    HeaderComponent() : Component("header-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-12">
            <div style="display:flex; justify-content:space-between; align-items:center;">
                <h1 style="margin:0; font-size:1.5rem;">My NovaCPP App</h1>
                <button type="button" class="theme-toggle-btn" onclick="window.toggleNovaTheme()" title="Toggle Theme">
                    <span class="theme-icon-light">&#9728;</span>
                    <span class="theme-icon-dark">&#9790;</span>
                </button>
            </div>
        </div>
        )html";
    }
};

class CounterCard : public np::Component {
public:
    CounterCard() : Component("counter-card") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-6">
            <h3>Interactive Counter</h3>
            <p>Current count: <strong>)html" + std::to_string(counter.get()) + R"html(</strong></p>
            <div class="btn-group">
                <button class="btn-secondary" nova-click="dec" nova-target="counter-card">&minus; Decrease</button>
                <button class="btn-primary" nova-click="inc" nova-target="counter-card">&plus; Increase</button>
            </div>
        </div>
        )html";
    }
};

class DatabaseCard : public np::Component {
public:
    DatabaseCard() : Component("db-card") {}

    void render(np::NovaBuilder& np) override {
        // Query database safely via pooled connection
        std::string userCount = "0";
        try {
            userCount = Database::queryScalar("SELECT COUNT(*) FROM users", "0");
        } catch (...) {}

        np << R"html(
        <div class="bento-card col-6">
            <h3>Database Engine</h3>
            <p>Registered users in SQLite WAL: <strong>)html" + userCount + R"html(</strong></p>
            <button class="btn-secondary" nova-click="refresh_db" nova-target="db-card">Refresh Count</button>
        </div>
        )html";
    }
};

// Instantiate components
HeaderComponent headerComp;
CounterCard counterCard;
DatabaseCard dbCard;

// ============================================================================
// 3. PAGE ROUTING & ACTION HANDLERS
// ============================================================================
void renderHomePage(np::NovaBuilder& np) {
    // Action handlers: triggered by nova-click attributes
    np.onClick("inc", []() {
        counter = counter + 1;
    });

    np.onClick("dec", []() {
        counter = counter - 1;
    });

    np.onClick("refresh_db", []() {
        // Connection pool automatically handles concurrency
    });

    // Form submission handler: triggered by nova-form="myForm"
    np.onForm("my_form", [](const std::map<std::string, std::string>& data) {
        if (data.count("text")) {
            message = "Received: " + data.at("text");
        }
    });

    // Render components inside the 12-column responsive Bento Grid
    np << "<div class='bento-grid'>";
    np.renderComponent(headerComp);
    np.renderComponent(counterCard);
    np.renderComponent(dbCard);
    np << "</div>";
}

void renderAboutPage(np::NovaBuilder& np) {
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
