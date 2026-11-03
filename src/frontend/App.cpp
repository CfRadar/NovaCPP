/* ============================================================================
   NOVACPP STARTER TEMPLATE & SHOWCASE
   A blazing-fast, reactive full-stack C++ web framework.
   Combines the DX of React with the raw speed of native C++.

   ============================================================================
   QUICK-START BOILERPLATE (To start coding your own app, wipe this file and use):
   ============================================================================
   #include <string>
   #include <vector>
   #include <map>
   #include <iostream>

   // All Framework Headers:
   #include "../../novacpp/html.hpp"        // Core builder, components, server
   #include "../../novacpp/state.hpp"       // Reactive session-isolated np::State hooks
   #include "../../novacpp/fetch.hpp"       // Native outbound REST client
   #include "../../novacpp/worker_pool.hpp" // Worker pool for async background tasks
   #include "../../novacpp/env.hpp"         // Zero-dependency .env loader
   #include "../backend/Database.hpp"       // SQLite WAL connection pool & queries
   #include "../backend/Auth.hpp"           // User authentication & sessions

   np::State<int> myCount(0);

   class MyComponent : public np::Component {
   public:
       MyComponent() : Component("my-comp") {}
       void render(np::NovaBuilder& np) override {
           np << "<div class='bento-card col-12'>"
              << "  <h2>My First NovaCPP Component</h2>"
              << "  <p>Count: " + std::to_string(myCount.get()) + "</p>"
              << "  <button class='btn-primary' nova-click='inc' nova-target='my-comp'>+ Count</button>"
              << "</div>";
       }
   };

   MyComponent myComp;

   void renderHomePage(np::RenderContext& np) {
       np.onClick("inc", []() { myCount = myCount + 1; });
       np << "<div class='bento-grid'>";
       np.renderComponent(myComp);
       np << "</div>";
   }

   void renderAboutPage(np::RenderContext& np) {
       np << "<div class='bento-grid'><div class='bento-card col-12'><h1>About</h1><a href='/' nova-link>&larr; Back</a></div></div>";
   }
   ============================================================================
   (A standalone version of this template is also available in src/frontend/Boilerplate.cpp)
   ============================================================================ */

#include <string>
#include <iostream>
#include <ctime>
#include "../../novacpp/html.hpp"
#include "../../novacpp/state.hpp"
#include "../../novacpp/fetch.hpp"
#include "../../novacpp/worker_pool.hpp"
#include "../backend/Database.hpp"
#include <vector>
#include <map>

// ===================================================================
// REACTIVE STATE MANAGEMENT
// `np::State<T>` creates thread-safe reactive variables. NovaCPP 
// automatically isolates these variables for each user session.
// ===================================================================
np::State<int> counter(0);
np::State<std::string> quote("Click any REST API button below to test network latency.");
np::State<int> bgJobsCompleted(0);
np::State<int> activeDocLesson(0);
np::State<int> sessionTouchCount(0);

// ===================================================================
// MODULAR COMPONENTS
// Inheriting from `np::Component` encapsulates UI markup and logic.
// Surgical DOM updates swap ONLY the targeted element, just like React!
// ===================================================================

class NavBarComponent : public np::Component {
public:
    NavBarComponent() : Component("navbar-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-12 navbar-card">
            <a href="/" class="brand-wrapper" nova-link>
                <img src="/NovaCPP.png" alt="NovaCPP" class="brand-logo-img">
                <span class="brand-title">NovaCPP</span>
            </a>
            <div class="nav-links">
                <a href="/" class="nav-link" nova-link>Home</a>
                <a href="/about" class="nav-link" nova-link>About</a>
                <a href="https://nova-documentation.vercel.app/" target="_blank" rel="noopener" class="nav-link">Docs &nearr;</a>
            </div>
            <div class="nav-right-cluster">
                <button type="button" class="theme-toggle-btn" onclick="window.toggleNovaTheme()" title="Toggle Light / Dark mode" aria-label="Toggle theme">
                    <span class="theme-icon-light">&#9728;</span>
                    <span class="theme-icon-dark">&#9790;</span>
                </button>
                <a href="/about" class="nav-status-badge" nova-link>
                    <span class="status-dot"></span>
                    <span>C++17 Engine &middot; Port 8080</span>
                </a>
            </div>
        </div>
        )html";
    }
};

class HeroComponent : public np::Component {
public:
    HeroComponent() : Component("hero-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-12 hero-card">
            <div class="hero-content">
                <div class="hero-text-block">
                    <div class="hero-badge-tag">FULL-STACK C++ MONOLITH</div>
                    <h1 class="hero-title">
                        Build Real-Time Web Apps with <span class="accent-text">C++ Speed &amp; React DX</span>
                    </h1>
                    <p class="hero-subtitle">
                        Zero API latency. Thread-local reactive state. Instant Fast Refresh hot reloading. 
                        Native npm packages pre-bundled via esbuild in milliseconds.
                    </p>
                    <div class="hero-actions">
                        <div class="hero-code-chip">
                            <span class="mono-prompt">$</span> npm run dev
                        </div>
                        <button class="btn-primary" data-nova-effect="confetti">
                            Launch Confetti &rarr;
                        </button>
                    </div>
                </div>
                <div class="hero-logo-wrapper">
                    <img src="/NovaCPP.png" alt="NovaCPP Framework" class="hero-logo">
                </div>
            </div>
        </div>
        )html";
    }
};

class ClockComponent : public np::Component {
public:
    ClockComponent() : Component("clock-comp") {}
    
    void render(np::NovaBuilder& np) override {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", ltm);
        std::string timeStr(buffer);

        np << R"html(
        <div class="bento-card col-4 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-live">LIVE POLLING</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <div class="counter-val">)html" + timeStr + R"html(</div>
                        <div class="card-desc">This C++ clock updates automatically every 1000ms without WebSockets.</div>
                    </div>
                </div>
                <!-- BACK: C++ COMPONENT SOURCE CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>ClockComponent.cpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Background live polling component</span>
<span class="syn-kw">class</span> <span class="syn-type">ClockComponent</span> : <span class="syn-kw">public</span> np::Component {
<span class="syn-kw">public</span>:
    <span class="syn-type">ClockComponent</span>() : Component(<span class="syn-str">"clock-comp"</span>) {}
    <span class="syn-kw">void</span> <span class="syn-fn">render</span>(np::NovaBuilder&amp; np) <span class="syn-kw">override</span> {
        time_t now = time(0);
        <span class="syn-type">char</span> buf[80];
        strftime(buf, <span class="syn-kw">sizeof</span>(buf), <span class="syn-str">"%H:%M:%S"</span>, localtime(&amp;now));
        np &lt;&lt; <span class="syn-str">"&lt;div class='val'&gt;"</span> + std::string(buf) + <span class="syn-str">"&lt;/div&gt;"</span>;
    }
};
<span class="syn-com">// Polled every 1000ms: np.renderComponent(clockComp, 1000);</span></code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class CounterComponent : public np::Component {
public:
    CounterComponent() : Component("counter-comp") {}
    
    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-4 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-blue">REACTIVE STATE HOOK</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <div class="counter-val">)html" + std::to_string(counter.get()) + R"html(</div>
                        <p class="card-desc">Server-isolated state stored safely in C++ RAM.</p>
                        <div class="btn-group">
                            <button class="btn-secondary" nova-click="decrement" nova-target="counter-comp">&minus; Decrease</button>
                            <button class="btn-primary" nova-click="increment" nova-target="counter-comp">&plus; Increase</button>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ COMPONENT SOURCE CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>CounterComponent.cpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Reactive state variable isolated per user session</span>
np::State&lt;<span class="syn-type">int</span>&gt; counter(0);

<span class="syn-kw">class</span> <span class="syn-type">CounterComponent</span> : <span class="syn-kw">public</span> np::Component {
<span class="syn-kw">public</span>:
    <span class="syn-type">CounterComponent</span>() : Component(<span class="syn-str">"counter-comp"</span>) {}
    <span class="syn-kw">void</span> <span class="syn-fn">render</span>(np::NovaBuilder&amp; np) <span class="syn-kw">override</span> {
        np &lt;&lt; <span class="syn-str">"&lt;div&gt;"</span> + std::to_string(counter.get()) + <span class="syn-str">"&lt;/div&gt;"</span>;
    }
};

<span class="syn-com">// Event bindings with surgical DOM replacement</span>
np.onClick(<span class="syn-str">"increment"</span>, []() { counter = counter + 1; });
np.onClick(<span class="syn-str">"decrement"</span>, []() { counter = counter - 1; });</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class FastRefreshCard : public np::Component {
public:
    FastRefreshCard() : Component("hmr-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-4 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-lavender">FAST REFRESH HMR</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <h3 class="card-title">Instant Hot Reloading</h3>
                        <p class="card-desc">
                            Edit <code>src/frontend/App.cpp</code> and press <kbd>Ctrl+S</kbd>. 
                            NovaCPP re-renders in ~1s without resetting your counter state!
                        </p>
                        <div class="btn-group">
                            <button class="btn-secondary" data-nova-effect="confetti">
                                Test npm Module
                            </button>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ COMPONENT SOURCE CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>FastRefresh.cpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Fast Refresh HMR Server Architecture</span>
<span class="syn-com">// 1. OS-level watcher detects file save in src/</span>
<span class="syn-com">// 2. Fast incremental build via MSVC / Clang (~1s)</span>
<span class="syn-com">// 3. Server sends SSE event: 'component-update'</span>
<span class="syn-com">// 4. Client surgically swaps DOM element without state loss</span>
<span class="syn-kw">class</span> <span class="syn-type">FastRefreshCard</span> : <span class="syn-kw">public</span> np::Component {
<span class="syn-kw">public</span>:
    <span class="syn-type">FastRefreshCard</span>() : Component(<span class="syn-str">"hmr-comp"</span>) {}
    <span class="syn-kw">void</span> <span class="syn-fn">render</span>(np::NovaBuilder&amp; np) <span class="syn-kw">override</span> {
        np &lt;&lt; <span class="syn-str">"&lt;h3&gt;Instant Hot Reloading&lt;/h3&gt;"</span>;
    }
};</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class AdvancedApiComponent : public np::Component {
public:
    AdvancedApiComponent() : Component("api-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-6 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-blue">REST CLIENT</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <h3 class="card-title">Native np::fetch Engine</h3>
                        <div class="quote-box">
                            <div class="quote-text">)html" + quote.get() + R"html(</div>
                        </div>
                        <div style="display: grid; grid-template-columns: repeat(4, 1fr); gap: 0.5rem; width: 100%;">
                            <button class="btn-secondary" nova-click="apiGet" nova-target="api-comp">GET</button>
                            <button class="btn-primary" nova-click="apiPost" nova-target="api-comp">POST</button>
                            <button class="btn-secondary" nova-click="apiPut" nova-target="api-comp">PUT</button>
                            <button class="btn-destructive" nova-click="apiDelete" nova-target="api-comp">DEL</button>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ COMPONENT SOURCE CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>FetchEngine.cpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Synchronous outbound REST client via WinHTTP / OpenSSL</span>
np.onClick(<span class="syn-str">"apiGet"</span>, []() {
    quote = np::fetch(<span class="syn-str">"https://jsonplaceholder.typicode.com/posts/1"</span>, <span class="syn-str">"GET"</span>);
});

np.onClick(<span class="syn-str">"apiPost"</span>, []() {
    <span class="syn-type">std::string</span> body = R"({"title":"NovaCPP","userId":1})";
    <span class="syn-type">std::string</span> headers = <span class="syn-str">"Content-Type: application/json\r\n"</span>;
    quote = np::fetch(<span class="syn-str">"https://jsonplaceholder.typicode.com/posts"</span>, <span class="syn-str">"POST"</span>, body, headers);
});</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class NpmShowcaseComponent : public np::Component {
public:
    NpmShowcaseComponent() : Component("npm-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-6 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-peach">NATIVE NPM COMPATIBILITY</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <h3 class="card-title">Pre-Bundled Browser Modules</h3>
                        <p class="card-desc">
                            Standard <code>npm i &lt;pkg&gt;</code> packages pre-bundled into ESM in ~40ms. 
                            Import bare specifiers directly in <code>render/app.js</code> via browser import maps!
                        </p>
                        <div class="btn-group">
                            <button class="btn-secondary btn-confetti" onclick="window.triggerNovaConfetti()">
                                Burst Confetti Particles &#127881;
                            </button>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ COMPONENT SOURCE CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">ESM</span>
                            <span>render/app.js</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Direct bare specifier ESM imports via Import Map</span>
<span class="syn-kw">import</span> confetti <span class="syn-kw">from</span> <span class="syn-str">'canvas-confetti'</span>;
<span class="syn-kw">import</span> dayjs <span class="syn-kw">from</span> <span class="syn-str">'dayjs'</span>;

window.<span class="syn-fn">triggerNovaConfetti</span> = () =&gt; {
    confetti({
        particleCount: 80,
        spread: 70,
        colors: [<span class="syn-str">'#5B7CFA'</span>, <span class="syn-str">'#93C5FD'</span>, <span class="syn-str">'#A7F3D0'</span>, <span class="syn-str">'#DDD6FE'</span>]
    });
};</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

// Reusable Info Block Component with Code Flip
class FeatureBlock : public np::Component {
private:
    std::string number;
    std::string title;
    std::string desc;
public:
    FeatureBlock(std::string id, std::string number, std::string title, std::string desc) 
        : Component(id), number(number), title(title), desc(desc) {}
    
    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-6 flip-card feature-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="feature-header">
                            <span class="feature-number">)html" + number + R"html(</span>
                            <h3 class="card-title">)html" + title + R"html(</h3>
                        </div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <p class="card-desc">)html" + desc + R"html(</p>
                    </div>
                </div>
                <!-- BACK: C++ COMPONENT SOURCE CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>FeatureBlock.cpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-kw">class</span> <span class="syn-type">FeatureBlock</span> : <span class="syn-kw">public</span> np::Component {
    <span class="syn-type">std::string</span> num, title, desc;
<span class="syn-kw">public</span>:
    <span class="syn-type">FeatureBlock</span>(<span class="syn-type">string</span> id, <span class="syn-type">string</span> n, <span class="syn-type">string</span> t, <span class="syn-type">string</span> d) 
        : Component(id), num(n), title(t), desc(d) {}
    <span class="syn-kw">void</span> <span class="syn-fn">render</span>(np::NovaBuilder&amp; np) <span class="syn-kw">override</span> {
        np &lt;&lt; <span class="syn-str">"&lt;h3&gt;"</span> + title + <span class="syn-str">"&lt;/h3&gt;&lt;p&gt;"</span> + desc + <span class="syn-str">"&lt;/p&gt;"</span>;
    }
};</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

// Instantiate our components globally
NavBarComponent navComp;
HeroComponent heroComp;
ClockComponent clockComp;
CounterComponent counterComp;
FastRefreshCard hmrComp;
AdvancedApiComponent apiComp;
NpmShowcaseComponent npmComp;

// Feature blocks
FeatureBlock speedFeat("feat-speed", "01", "Sub-Millisecond RAM Speed", "Ditch slow Node.js serialization. Execute frontend logic at native C++ memory speed.");
FeatureBlock spaFeat("feat-spa", "02", "Surgical SPA Navigation", "Seamless transitions. NovaCPP surgically updates only changed DOM elements with zero full page reloads.");
FeatureBlock stateFeat("feat-state", "03", "Thread-Local State Isolation", "Every user receives an isolated, thread-safe session copy of all reactive np::State variables.");
FeatureBlock monolithFeat("feat-monolith", "04", "Full-Stack Single Binary", "Frontend UI, REST clients, and backend services compile into a single deployable native binary.");

class DatabaseShowcaseComponent : public np::Component {
public:
    DatabaseShowcaseComponent() : Component("db-comp") {}

    void render(np::NovaBuilder& np) override {
        std::vector<std::map<std::string, std::string>> users;
        try {
            users = Database::query("SELECT id, name, email, role FROM users ORDER BY id DESC LIMIT 5");
        } catch (...) {
            // Graceful fallback if database unavailable
        }

        std::string rowsHtml = "";
        for (const auto& u : users) {
            std::string id = u.count("id") ? u.at("id") : "0";
            std::string name = u.count("name") ? u.at("name") : "";
            std::string email = u.count("email") ? u.at("email") : "";
            std::string role = u.count("role") ? u.at("role") : "developer";

            rowsHtml += "<tr>";
            rowsHtml += "<td>#" + id + "</td>";
            rowsHtml += "<td><strong>" + name + "</strong></td>";
            rowsHtml += "<td><code>" + email + "</code></td>";
            rowsHtml += "<td><span class='table-badge'>" + role + "</span></td>";
            rowsHtml += "</tr>";
        }

        if (rowsHtml.empty()) {
            rowsHtml = "<tr><td colspan='4' style='text-align:center; color: var(--text-muted);'>No user records found. Click 'Add Developer' to create one.</td></tr>";
        }

        np << R"html(
        <div class="bento-card col-8 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-mint">SQLITE WAL DATABASE</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <h3 class="card-title">Live SQLite Query Engine</h3>
                        <p class="card-desc">
                            Direct C++ query execution via RAII connection pool. Reads and writes execute in WAL journal mode without blocking.
                        </p>
                        <div class="table-container">
                            <table class="nova-table">
                                <thead>
                                    <tr>
                                        <th>ID</th>
                                        <th>Developer</th>
                                        <th>Email</th>
                                        <th>Role</th>
                                    </tr>
                                </thead>
                                <tbody>
        )html" + rowsHtml + R"html(
                                </tbody>
                            </table>
                        </div>
                        <div class="btn-group">
                            <button class="btn-primary" nova-click="add_sample_user" nova-target="db-comp">&plus; Add Developer</button>
                            <button class="btn-secondary" nova-click="reset_sample_users" nova-target="db-comp">Reset Seed Data</button>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>DatabaseShowcase.cpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Direct pooled query execution with RAII safety</span>
<span class="syn-kw">auto</span> rows = Database::query(
    <span class="syn-str">"SELECT id, name, email, role FROM users ORDER BY id DESC LIMIT 5"</span>
);

<span class="syn-com">// Insert mutation via connection pool guard</span>
Database::execute(
    <span class="syn-str">"INSERT INTO users (name, email, role) VALUES ('Grace Hopper', 'grace@novacpp.dev', 'core');"</span>
);</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class DbPoolTelemetryComponent : public np::Component {
public:
    DbPoolTelemetryComponent() : Component("dbpool-comp") {}

    void render(np::NovaBuilder& np) override {
        size_t cap = Database::getPoolCapacity();
        size_t avail = Database::getAvailableConnections();
        size_t active = cap > avail ? (cap - avail) : 0;

        np << R"html(
        <div class="bento-card col-4 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-blue">RAII CONNECTION POOL</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <h3 class="card-title">Pool Telemetry</h3>
                        <p class="card-desc">Bounded connection pool preventing database handle exhaustion.</p>
                        <div class="metric-grid">
                            <div class="metric-card">
                                <span class="metric-value">)html" + std::to_string(cap) + R"html(</span>
                                <span class="metric-label">Capacity</span>
                            </div>
                            <div class="metric-card">
                                <span class="metric-value">)html" + std::to_string(avail) + R"html(</span>
                                <span class="metric-label">Available</span>
                            </div>
                            <div class="metric-card">
                                <span class="metric-value">)html" + std::to_string(active) + R"html(</span>
                                <span class="metric-label">In Use</span>
                            </div>
                        </div>
                        <div style="font-size:0.8rem; color:var(--text-secondary); margin-top:0.5rem; display:flex; align-items:center; gap:6px;">
                            <span class="status-pulse-green"></span>
                            <span>WAL Journal Mode: ACTIVE</span>
                        </div>
                        <div class="btn-group" style="margin-top:1rem;">
                            <button class="btn-secondary" nova-click="test_pool_acquire" nova-target="dbpool-comp">Acquire Guard</button>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>ConnectionPool.hpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Acquire connection with automatic RAII return</span>
{
    <span class="syn-kw">auto</span> guard = Database::acquire(std::chrono::milliseconds(1500));
    guard-&gt;execute(<span class="syn-str">"PRAGMA wal_checkpoint(PASSIVE);"</span>);
} <span class="syn-com">// Handle returned automatically on scope exit</span></code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class WorkerPoolShowcaseComponent : public np::Component {
public:
    WorkerPoolShowcaseComponent() : Component("worker-comp") {}

    void render(np::NovaBuilder& np) override {
        size_t threads = np::WorkerPool::defaultPool().threadCount();
        size_t pending = np::WorkerPool::defaultPool().pendingTasks();
        int completed = bgJobsCompleted.get();

        np << R"html(
        <div class="bento-card col-4 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-lavender">WORKER THREAD POOL</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <h3 class="card-title">Async Task Dispatcher</h3>
                        <p class="card-desc">Offload intensive CPU/IO tasks from HTTP event loop.</p>
                        <div class="metric-grid">
                            <div class="metric-card">
                                <span class="metric-value">)html" + std::to_string(threads) + R"html(</span>
                                <span class="metric-label">Threads</span>
                            </div>
                            <div class="metric-card">
                                <span class="metric-value">)html" + std::to_string(pending) + R"html(</span>
                                <span class="metric-label">Pending</span>
                            </div>
                            <div class="metric-card">
                                <span class="metric-value">)html" + std::to_string(completed) + R"html(</span>
                                <span class="metric-label">Completed</span>
                            </div>
                        </div>
                        <div class="btn-group" style="margin-top:1rem;">
                            <button class="btn-primary" nova-click="dispatch_bg_task" nova-target="worker-comp">Dispatch Task</button>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>WorkerPool.hpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Enqueue non-blocking background job</span>
np.enqueueTask([]() {
    <span class="syn-com">// Heavy computation in worker thread</span>
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
});</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class SessionStoreComponent : public np::Component {
public:
    SessionStoreComponent() : Component("session-comp") {}

    void render(np::NovaBuilder& np) override {
        bool hasRedis = np::hasEnv("REDIS_URL");
        std::string mode = hasRedis ? "Distributed Redis" : "Thread-Local RAM";
        int touches = sessionTouchCount.get();

        np << R"html(
        <div class="bento-card col-4 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-peach">SESSION STORE</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <h3 class="card-title">Distributed Session State</h3>
                        <p class="card-desc">Session isolation with automatic sliding TTL expiration.</p>
                        <div class="metric-grid">
                            <div class="metric-card">
                                <span class="metric-value">60s</span>
                                <span class="metric-label">Sliding TTL</span>
                            </div>
                            <div class="metric-card">
                                <span class="metric-value" style="font-size:1.1rem; padding-top:4px;">)html" + std::to_string(touches) + R"html( hits</span>
                                <span class="metric-label">Interactions</span>
                            </div>
                        </div>
                        <div style="font-size:0.8rem; color:var(--text-secondary); margin-top:0.5rem; display:flex; align-items:center; gap:6px;">
                            <span class="status-pulse-green"></span>
                            <span>Mode: )html" + mode + R"html(</span>
                        </div>
                        <div class="btn-group" style="margin-top:1rem;">
                            <button class="btn-secondary" nova-click="touch_session" nova-target="session-comp">Touch Session</button>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>RedisStateStore.hpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Connect distributed multi-pod session store</span>
<span class="syn-kw">if</span> (<span class="syn-kw">const</span> <span class="syn-type">char</span>* url = std::getenv(<span class="syn-str">"REDIS_URL"</span>)) {
    np::useStateStore(np::RedisStateStore::fromUri(url));
}</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class HealthMetricsComponent : public np::Component {
public:
    HealthMetricsComponent() : Component("health-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-4 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-mint">SECURITY &amp; PROBES</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <h3 class="card-title">Production Health Probes</h3>
                        <p class="card-desc">Zero-overhead orchestration endpoints for Kubernetes &amp; Docker.</p>
                        <div style="display:flex; flex-direction:column; gap:8px; margin: 12px 0;">
                            <div style="display:flex; justify-content:space-between; font-size:0.85rem; padding: 6px 10px; background:var(--bg-subtle); border-radius:6px;">
                                <code>/healthz (Liveness)</code>
                                <span style="color:#10B981; font-weight:600;">200 OK</span>
                            </div>
                            <div style="display:flex; justify-content:space-between; font-size:0.85rem; padding: 6px 10px; background:var(--bg-subtle); border-radius:6px;">
                                <code>/readyz (Readiness)</code>
                                <span style="color:#10B981; font-weight:600;">200 OK</span>
                            </div>
                            <div style="display:flex; justify-content:space-between; font-size:0.85rem; padding: 6px 10px; background:var(--bg-subtle); border-radius:6px;">
                                <code>Security Headers</code>
                                <span style="color:#10B981; font-weight:600;">CSP + HSTS</span>
                            </div>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>Probes.hpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Kubernetes readiness probe handler</span>
svr.Get(<span class="syn-str">"/readyz"</span>, [](<span class="syn-kw">const</span> httplib::Request&amp;, httplib::Response&amp; res) {
    <span class="syn-kw">if</span> (Database::isConnected()) {
        res.set_content(<span class="syn-str">"ready"</span>, <span class="syn-str">"text/plain"</span>);
    } <span class="syn-kw">else</span> {
        res.status = 503;
    }
});</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class ArchitectureMapComponent : public np::Component {
public:
    ArchitectureMapComponent() : Component("arch-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-12 flip-card">
            <div class="flip-card-inner">
                <!-- FRONT: INTERACTIVE PREVIEW -->
                <div class="card-face card-front">
                    <div class="card-topbar">
                        <div class="badge badge-blue">SYSTEM TOPOLOGY</div>
                        <button type="button" class="btn-flip" onclick="window.flipCard(this)" title="View C++ Code" aria-label="View C++ Code">
                            &lt;/&gt; Code
                        </button>
                    </div>
                    <div class="card-body">
                        <h3 class="card-title">Full-Stack C++ Architecture</h3>
                        <p class="card-desc">
                            NovaCPP unifies high-performance C++ backend services with reactive browser rendering into a single, cohesive binary.
                        </p>
                        <div class="arch-grid">
                            <div class="arch-node">
                                <span class="badge badge-peach" style="align-self:flex-start; margin-bottom:4px;">01 CLIENT</span>
                                <span class="arch-node-title">Browser Engine</span>
                                <span class="arch-node-desc">Surgical DOM morphing, esbuild import map bundling, and zero-flicker state preservation.</span>
                            </div>
                            <div class="arch-node">
                                <span class="badge badge-blue" style="align-self:flex-start; margin-bottom:4px;">02 SERVER</span>
                                <span class="arch-node-title">C++17 Core</span>
                                <span class="arch-node-desc">Multi-threaded HTTP engine, thread-safe routing tree, and live polling channels.</span>
                            </div>
                            <div class="arch-node">
                                <span class="badge badge-mint" style="align-self:flex-start; margin-bottom:4px;">03 STORAGE</span>
                                <span class="arch-node-title">SQLite WAL Pool</span>
                                <span class="arch-node-desc">RAII-guarded connection pool, sub-millisecond writes, and zero file lock violations.</span>
                            </div>
                            <div class="arch-node">
                                <span class="badge badge-lavender" style="align-self:flex-start; margin-bottom:4px;">04 COMPUTE</span>
                                <span class="arch-node-title">Worker Thread Pool</span>
                                <span class="arch-node-desc">Asynchronous task dispatcher isolating heavy workloads from HTTP response latency.</span>
                            </div>
                        </div>
                    </div>
                </div>
                <!-- BACK: C++ CODE -->
                <div class="card-face card-back">
                    <div class="card-topbar">
                        <div class="code-title-badge">
                            <span class="badge-lang">C++17</span>
                            <span>NovaMonolith.cpp</span>
                        </div>
                        <div class="card-actions">
                            <button type="button" class="btn-copy-code" onclick="window.copyCardCode(this)" title="Copy Code">Copy</button>
                            <button type="button" class="btn-flip btn-flip-back" onclick="window.flipCard(this)" title="Back to Preview">&times; Preview</button>
                        </div>
                    </div>
                    <div class="code-container">
                        <pre class="code-pre"><code><span class="syn-com">// Unified single-binary entrypoint</span>
<span class="syn-type">int</span> main() {
    np::loadEnv(<span class="syn-str">".env"</span>);
    Database::connect();
    np::NovaBuilder np;
    np.route(<span class="syn-str">"/"</span>, renderHomePage);
    np.listen(8080);
    <span class="syn-kw">return</span> 0;
}</code></pre>
                    </div>
                </div>
            </div>
        </div>
        )html";
    }
};

class DocsBrowserComponent : public np::Component {
public:
    DocsBrowserComponent() : Component("docs-comp") {}

    void render(np::NovaBuilder& np) override {
        int topic = activeDocLesson.get();
        if (topic < 0 || topic > 4) topic = 0;

        std::string title = "";
        std::string desc = "";
        std::string snippet = "";

        if (topic == 0) {
            title = "Lesson 01: Getting Started &amp; Project Scaffolding";
            desc = "Scaffold a production-grade NovaCPP application in seconds using the official CLI scaffolder. Builds a pre-configured project with CMake, live hot reloading, and npm support.";
            snippet = "$ npm create novacpp@latest my-app\n$ cd my-app\n$ npm install\n$ npm run dev";
        } else if (topic == 1) {
            title = "Lesson 02: Modular UI Components";
            desc = "Components encapsulate HTML generation and event handlers. Inherit from np::Component, implement the render method, and register surgical DOM swap targets.";
            snippet = "class MyCard : public np::Component {\npublic:\n    MyCard() : Component(\"my-card\") {}\n    void render(np::NovaBuilder& np) override {\n        np << \"<div class='card'>Hello World</div>\";\n    }\n};";
        } else if (topic == 2) {
            title = "Lesson 03: Reactive State &amp; Event Handlers";
            desc = "np::State<T> manages thread-safe variables isolated per user session. Bind events using np.onClick or np.onForm with automatic surgical DOM updates.";
            snippet = "np::State<int> counter(0);\n\nnp.onClick(\"increment\", []() {\n    counter = counter + 1;\n});";
        } else if (topic == 3) {
            title = "Lesson 04: Database &amp; RAII Connection Pooling";
            desc = "Access SQLite with WAL journaling mode using Database::acquire(). Scoped guards automatically return connections to the pool, preventing deadlocks.";
            snippet = "{\n    auto guard = Database::acquire();\n    auto rows = guard->query(\"SELECT * FROM users\");\n}";
        } else {
            title = "Lesson 05: Production Deployment &amp; Kubernetes";
            desc = "Deploy as a multi-pod cluster using Redis session synchronization, non-root Docker scratch images, and native /healthz and /readyz liveness probes.";
            snippet = "# Docker multi-stage build\nFROM gcc:13 AS builder\n# Compiles into static ~3.8MB binary\nEXPOSE 8080\nCMD [\"./NovaCPP\"]";
        }

        std::string t0_act = (topic == 0) ? " active" : "";
        std::string t1_act = (topic == 1) ? " active" : "";
        std::string t2_act = (topic == 2) ? " active" : "";
        std::string t3_act = (topic == 3) ? " active" : "";
        std::string t4_act = (topic == 4) ? " active" : "";

        np << R"html(
        <div class="bento-card col-8">
            <div class="card-topbar">
                <div class="badge badge-lavender">INTERACTIVE DOCS</div>
                <a href="https://nova-documentation.vercel.app/" target="_blank" rel="noopener" class="nav-link" style="font-size:0.8rem; font-weight:600;">Full Docs Site &nearr;</a>
            </div>
            <div class="card-body">
                <div class="docs-topic-tabs">
                    <button class="docs-topic-btn)html" + t0_act + R"html(" nova-click="switch_doc_0" nova-target="docs-comp">01 Scaffolder</button>
                    <button class="docs-topic-btn)html" + t1_act + R"html(" nova-click="switch_doc_1" nova-target="docs-comp">02 Components</button>
                    <button class="docs-topic-btn)html" + t2_act + R"html(" nova-click="switch_doc_2" nova-target="docs-comp">03 State</button>
                    <button class="docs-topic-btn)html" + t3_act + R"html(" nova-click="switch_doc_3" nova-target="docs-comp">04 Database</button>
                    <button class="docs-topic-btn)html" + t4_act + R"html(" nova-click="switch_doc_4" nova-target="docs-comp">05 Production</button>
                </div>
                <div class="docs-lesson-box">
                    <div class="docs-lesson-title">)html" + title + R"html(</div>
                    <div class="docs-lesson-text">)html" + desc + R"html(</div>
                    <pre class="code-pre" style="margin:0;"><code>)html" + snippet + R"html(</code></pre>
                </div>
            </div>
        </div>
        )html";
    }
};

class BenchmarkSpecsComponent : public np::Component {
public:
    BenchmarkSpecsComponent() : Component("specs-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-4">
            <div class="card-topbar">
                <div class="badge badge-mint">BENCHMARKS</div>
            </div>
            <div class="card-body">
                <h3 class="card-title">Empirical Performance</h3>
                <p class="card-desc">Compiled native C++17 vs interpreted runtime stacks.</p>
                <div class="metric-grid">
                    <div class="metric-card">
                        <span class="metric-value">&lt; 5ms</span>
                        <span class="metric-label">Cold Start</span>
                    </div>
                    <div class="metric-card">
                        <span class="metric-value">4.1 MB</span>
                        <span class="metric-label">RAM Footprint</span>
                    </div>
                    <div class="metric-card">
                        <span class="metric-value">3.8 MB</span>
                        <span class="metric-label">Binary Size</span>
                    </div>
                    <div class="metric-card">
                        <span class="metric-value">&gt; 100k</span>
                        <span class="metric-label">Req / Sec</span>
                    </div>
                </div>
                <div style="margin-top:1rem;">
                    <a href="https://nova-documentation.vercel.app/" target="_blank" rel="noopener" class="btn-primary" style="text-decoration:none; display:flex; justify-content:center;">
                        Browse Official Documentation &rarr;
                    </a>
                </div>
            </div>
        </div>
        )html";
    }
};

// Global component instances
DatabaseShowcaseComponent dbComp;
DbPoolTelemetryComponent dbPoolComp;
WorkerPoolShowcaseComponent workerComp;
SessionStoreComponent sessionComp;
HealthMetricsComponent healthComp;
ArchitectureMapComponent archComp;
DocsBrowserComponent docsComp;
BenchmarkSpecsComponent specsComp;

class AboutComponent : public np::Component {
public:
    AboutComponent() : Component("about-comp") {}
    
    void render(np::NovaBuilder& np) override {
        np << R"html(
        <div class="bento-card col-12 about-hero-card">
            <h1 class="about-title">About NovaCPP</h1>
            <p class="about-desc">
                NovaCPP is an ultra-fast, modern C++ web framework engineered to deliver the developer experience 
                of React straight to the backend. It completely eliminates the need for Node.js at runtime, 
                allowing you to build reactive Single Page Applications (SPAs) purely in native C++.
            </p>
            <p class="about-desc">
                Key architectural pillars include <strong>Surgical DOM Swapping</strong>, 
                <strong>Automatic Live Polling</strong>, 
                <strong>Thread-Local Session State</strong>, and 
                <strong>Native npm Package Compatibility</strong> via esbuild import maps.
            </p>
            <div style="margin-top: 1.5rem;">
                <a href="/" class="btn-primary" style="text-decoration: none; padding: 0.75rem 1.6rem; display: inline-flex;" nova-link>
                    &larr; Back to Home
                </a>
            </div>
        </div>
        )html";
    }
};

AboutComponent aboutComp;

// ===================================================================
// PAGE ROUTING
// ===================================================================

void renderHomePage(np::RenderContext& np) {
    
    np.onLoad([]() {
        counter = 0;
        quote = "Select a REST API method to test network connectivity.";
    });

    np.onClick("increment", []() { 
        counter = counter + 1; 
    });
    
    np.onClick("decrement", []() { 
        counter = counter - 1; 
    });

    np.onClick("apiGet", []() {
        quote = "Fetching GET...";
        std::string res = np::fetch("https://jsonplaceholder.typicode.com/posts/1", "GET");
        if (res.find("\"id\": 1") != std::string::npos) quote = "GET Success!\n" + res.substr(0, 50) + "...";
        else quote = "GET Failed!";
    });

    np.onClick("apiPost", []() {
        quote = "Sending POST...";
        std::string body = R"({"title":"NovaCPP","body":"REST Client","userId":1})";
        std::string headers = "Content-Type: application/json\r\n";
        std::string res = np::fetch("https://jsonplaceholder.typicode.com/posts", "POST", body, headers);
        if (res.find("\"id\": 101") != std::string::npos) quote = "POST Created Successfully!\n" + res;
        else quote = "POST Failed!";
    });

    np.onClick("apiPut", []() {
        quote = "Sending PUT...";
        std::string body = R"({"id":1,"title":"Updated","body":"Data","userId":1})";
        std::string headers = "Content-Type: application/json\r\n";
        std::string res = np::fetch("https://jsonplaceholder.typicode.com/posts/1", "PUT", body, headers);
        if (res.find("\"title\": \"Updated\"") != std::string::npos) quote = "PUT Updated Successfully!";
        else quote = "PUT Failed!";
    });

    np.onClick("apiDelete", []() {
        quote = "Sending DELETE...";
        std::string res = np::fetch("https://jsonplaceholder.typicode.com/posts/1", "DELETE");
        if (res.find("{}") != std::string::npos || res.find("{\n}") != std::string::npos) quote = "DELETE Successful!";
        else quote = "DELETE Failed! Response: " + res;
    });

    // Form submission action handler demonstrating two-way form binding
    np.onForm("subscribe", [](const std::map<std::string, std::string>& data) {
        std::string email = data.count("email") ? data.at("email") : "anonymous";
        quote = "Subscribed successfully: " + email;
    });

    // Backend interactive actions
    np.onClick("add_sample_user", []() {
        static int s_userCount = 100;
        s_userCount++;
        std::string name = "Developer " + std::to_string(s_userCount);
        std::string email = "dev" + std::to_string(s_userCount) + "@novacpp.dev";
        std::string role = (s_userCount % 2 == 0) ? "developer" : "core-team";
        try {
            Database::execute("INSERT INTO users (name, email, role) VALUES (?, ?, ?);", name, email, role);
        } catch (...) {}
    });

    np.onClick("reset_sample_users", []() {
        try {
            Database::transaction([](np::SQLiteConnection& db) {
                db.execute("DELETE FROM users;");
                db.execute("INSERT INTO users (name, email, role) VALUES (?, ?, ?);", "Ada Lovelace", "ada@novacpp.dev", "architect");
                db.execute("INSERT INTO users (name, email, role) VALUES (?, ?, ?);", "Alan Turing", "alan@novacpp.dev", "core-dev");
                db.execute("INSERT INTO users (name, email, role) VALUES (?, ?, ?);", "Margaret Hamilton", "margaret@novacpp.dev", "lead");
            });
        } catch (...) {}
    });

    np.onClick("test_pool_acquire", []() {
        try {
            auto guard = Database::acquire(std::chrono::milliseconds(1500));
            guard->execute("PRAGMA wal_checkpoint(PASSIVE);");
        } catch (...) {}
    });

    np.onClick("dispatch_bg_task", []() {
        np::WorkerPool::defaultPool().enqueue([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            bgJobsCompleted = bgJobsCompleted.get() + 1;
        });
    });

    np.onClick("async_job", []() {
        np::WorkerPool::defaultPool().enqueue([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        });
    });

    np.onClick("touch_session", []() {
        sessionTouchCount = sessionTouchCount.get() + 1;
    });

    np.onClick("switch_doc_0", []() { activeDocLesson = 0; });
    np.onClick("switch_doc_1", []() { activeDocLesson = 1; });
    np.onClick("switch_doc_2", []() { activeDocLesson = 2; });
    np.onClick("switch_doc_3", []() { activeDocLesson = 3; });
    np.onClick("switch_doc_4", []() { activeDocLesson = 4; });

    // TOP NAVBAR
    np << R"html(<div class="bento-grid" style="margin-bottom: var(--space-2);">)html";
    np.renderComponent(navComp);
    np << R"html(</div>)html";

    // STICKY SEGMENTED CONTROL TAB BAR
    np << R"html(
    <div class="nav-tab-slider-wrapper">
        <button type="button" class="swipe-arrow swipe-arrow-prev" onclick="window.swipePrev()" title="Previous Section" aria-label="Previous Section">&larr;</button>
        <div class="nav-tab-slider" role="tablist">
            <div class="nav-tab-pill"></div>
            <button type="button" class="tab-btn active" onclick="window.goToSlide(0)" role="tab" aria-selected="true" id="tab-0">
                <span class="tab-num">01</span> Frontend Engine
            </button>
            <button type="button" class="tab-btn" onclick="window.goToSlide(1)" role="tab" aria-selected="false" id="tab-1">
                <span class="tab-num">02</span> Backend Architecture
            </button>
            <button type="button" class="tab-btn" onclick="window.goToSlide(2)" role="tab" aria-selected="false" id="tab-2">
                <span class="tab-num">03</span> Architecture &amp; Docs
            </button>
        </div>
        <button type="button" class="swipe-arrow swipe-arrow-next" onclick="window.swipeNext()" title="Next Section" aria-label="Next Section">&rarr;</button>
    </div>

    <!-- Horizontal Swipe Viewport & Track -->
    <div class="nova-viewport">
        <div class="nova-swipe-track">
            
            <!-- SLIDE 0: FRONTEND ENGINE -->
            <div class="nova-slide" data-slide="frontend">
                <div class="section-banner">
                    <div>
                        <h2 class="section-banner-title">
                            <span class="badge badge-blue">SLIDE 01</span>
                            Frontend Engine &amp; Reactive UI
                        </h2>
                        <p class="section-banner-desc">Surgical DOM morphing, session-isolated state hooks, background polling, and pre-bundled npm ESM modules.</p>
                    </div>
                </div>
                <div class="bento-grid">
    )html";

    // Hero Showcase
    np.renderComponent(heroComp);

    // Row: Live Polling Clock, Reactive Counter, Fast Refresh HMR
    np.renderComponent(clockComp, 1000); 
    np.renderComponent(counterComp);
    np.renderComponent(hmrComp);

    // Row: API & npm Capabilities
    np.renderComponent(apiComp);
    np.renderComponent(npmComp);

    // Pillars
    np.renderComponent(speedFeat);
    np.renderComponent(spaFeat);

    np << R"html(
                </div>
            </div>

            <!-- SLIDE 1: BACKEND ARCHITECTURE -->
            <div class="nova-slide" data-slide="backend">
                <div class="section-banner">
                    <div>
                        <h2 class="section-banner-title">
                            <span class="badge badge-mint">SLIDE 02</span>
                            Backend Core &amp; Data Pipeline
                        </h2>
                        <p class="section-banner-desc">Thread-safe SQLite connection pool with WAL journaling, asynchronous worker threads, and distributed Redis session storage.</p>
                    </div>
                </div>
                <div class="bento-grid">
    )html";

    // SQLite Table & Telemetry
    np.renderComponent(dbComp);
    np.renderComponent(dbPoolComp);

    // Workers, Sessions & Health Probes
    np.renderComponent(workerComp);
    np.renderComponent(sessionComp);
    np.renderComponent(healthComp);

    // Pillars
    np.renderComponent(stateFeat);
    np.renderComponent(monolithFeat);

    np << R"html(
                </div>
            </div>

            <!-- SLIDE 2: ARCHITECTURE & DOCUMENTATION -->
            <div class="nova-slide" data-slide="docs">
                <div class="section-banner">
                    <div>
                        <h2 class="section-banner-title">
                            <span class="badge badge-lavender">SLIDE 03</span>
                            Architecture &amp; Documentation
                        </h2>
                        <p class="section-banner-desc">Full-stack system topology, interactive lesson browser, and empirical C++ performance specifications.</p>
                    </div>
                </div>
                <div class="bento-grid">
    )html";

    // Topology Map
    np.renderComponent(archComp);

    // Interactive Docs & Specs
    np.renderComponent(docsComp);
    np.renderComponent(specsComp);

    // Full About Hero Card
    np.renderComponent(aboutComp);

    np << R"html(
                </div>
            </div>

        </div>
    </div>
    )html";
}

void renderAboutPage(np::RenderContext& np) {
    np << R"html(<div class="bento-grid">)html";
    np.renderComponent(navComp);
    np.renderComponent(aboutComp);
    np << R"html(</div>)html";
}
