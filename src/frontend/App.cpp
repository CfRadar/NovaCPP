/* ============================================================================
   NOVACPP STARTER TEMPLATE
   A blazing-fast, reactive full-stack C++ web framework.
   Combines the DX of React with the raw speed of native C++.
   ============================================================================ */

#include <string>
#include <iostream>
#include <ctime>
#include "../../novacpp/html.hpp"
#include "../../novacpp/state.hpp"
#include "../../novacpp/fetch.hpp"

// ===================================================================
// REACTIVE STATE MANAGEMENT
// `np::State<T>` creates thread-safe reactive variables. NovaCPP 
// automatically isolates these variables for each user session.
// ===================================================================
np::State<int> counter(0);
np::State<std::string> quote("Click any REST API button below to test network latency.");

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

// ===================================================================
// PAGE ROUTING
// ===================================================================

void renderHomePage(np::NovaBuilder& np) {
    
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

    // Render the Home Page Layout (12-Column Responsive Bento Grid)
    np << R"html(<div class="bento-grid">)html";
    
    // ROW 1: Navigation
    np.renderComponent(navComp);
    
    // ROW 2: Hero Showcase
    np.renderComponent(heroComp);

    // ROW 3: Interactive Reactive Components
    np.renderComponent(clockComp, 1000); 
    np.renderComponent(counterComp);
    np.renderComponent(hmrComp);

    // ROW 4: API & npm Capabilities
    np.renderComponent(apiComp);
    np.renderComponent(npmComp);
    
    // ROW 5: Architectural Features
    np.renderComponent(speedFeat);
    np.renderComponent(spaFeat);
    np.renderComponent(stateFeat);
    np.renderComponent(monolithFeat);
    
    np << R"html(</div>)html";
}

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

void renderAboutPage(np::NovaBuilder& np) {
    np << R"html(<div class="bento-grid">)html";
    np.renderComponent(navComp);
    np.renderComponent(aboutComp);
    np << R"html(</div>)html";
}
