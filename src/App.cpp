#include <string>
#include <iostream>
#include <ctime>
#include "../novacpp/html.hpp"
#include "../novacpp/state.hpp"
#include "../novacpp/fetch.hpp"

// ===================================================================
// STATE MANAGEMENT
// `np::State` creates reactive variables. NovaCPP automatically isolates 
// these variables for every user connected to the server.
// ===================================================================
np::State<int> counter(0);
np::State<std::string> quote("Click the button to receive wisdom.");


// ===================================================================
// MODULAR COMPONENTS
// By inheriting from `np::Component`, you can split your UI into small, 
// independent pieces. When state changes, NovaCPP will surgically 
// update ONLY the component that needs to change, exactly like React!
// ===================================================================

class NavBarComponent : public np::Component {
public:
    NavBarComponent() : Component("navbar-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"(
        <div class="bento-card col-12 navbar-card">
            <a href="/" class="nav-link" nova-link>Home</a>
            <a href="/about" class="nav-link" nova-link>About</a>
            <div style="margin-left: auto; font-size: 0.8rem; color: #86868b; font-weight: 600; letter-spacing: 1px;">NOVACPP ENGINE</div>
        </div>
        )";
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

        np << R"(
        <div class="bento-card col-4">
            <div class="badge badge-live">LIVE POLLING</div>
            <div>
                <div class="counter-val">)" + timeStr + R"(</div>
                <div class="card-desc">This C++ clock updates automatically without WebSockets.</div>
            </div>
        </div>
        )";
    }
};

class CounterComponent : public np::Component {
public:
    CounterComponent() : Component("counter-comp") {}
    
    void render(np::NovaBuilder& np) override {
        np << R"(
        <div class="bento-card col-4">
            <div class="badge">STATE HOOK</div>
            <div class="counter-val">)" + std::to_string(counter.get()) + R"(</div>
            <div class="btn-group">
                <button class="btn-secondary" nova-click="decrement" nova-target="counter-comp">- Decrease</button>
                <button class="btn-primary" nova-click="increment" nova-target="counter-comp">+ Increase</button>
            </div>
        </div>
        )";
    }
};

class AdvancedApiComponent : public np::Component {
public:
    AdvancedApiComponent() : Component("api-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"(
        <div class="bento-card col-4">
            <div class="badge">ADVANCED REST API TEST</div>
            <div class="quote-box" style="margin-bottom: 0.8rem; height: 100px;">
                <div class="quote-text" style="-webkit-line-clamp: 4;">)" + quote.get() + R"(</div>
            </div>
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 0.5rem; width: 100%;">
                <button class="btn-secondary" nova-click="apiGet" nova-target="api-comp">GET</button>
                <button class="btn-primary" nova-click="apiPost" nova-target="api-comp">POST</button>
                <button class="btn-secondary" nova-click="apiPut" nova-target="api-comp">PUT</button>
                <button class="btn-primary" style="background: #ff3b30;" nova-click="apiDelete" nova-target="api-comp">DELETE</button>
            </div>
        </div>
        )";
    }
};

// Reusable Info Block Component (No Emojis, Clean Typography)
class FeatureBlock : public np::Component {
private:
    std::string number;
    std::string title;
    std::string desc;
public:
    FeatureBlock(std::string id, std::string number, std::string title, std::string desc) 
        : Component(id), number(number), title(title), desc(desc) {}
    
    void render(np::NovaBuilder& np) override {
        np << R"(
        <div class="bento-card col-4">
            <div class="feature-number">)" + number + R"(</div>
            <div>
                <h3 class="card-title">)" + title + R"(</h3>
                <p class="card-desc">)" + desc + R"(</p>
            </div>
        </div>
        )";
    }
};


// Instantiate our components globally
NavBarComponent navComp;
ClockComponent clockComp;
CounterComponent counterComp;
AdvancedApiComponent apiComp;

// Feature blocks
FeatureBlock speedFeat("feat-speed", "01", "Blazing Fast", "Compiled C++ backend provides absolute millisecond response times.");
FeatureBlock spaFeat("feat-spa", "02", "SPA Routing", "Seamless page transitions without any browser reloads or flashing.");
FeatureBlock stateFeat("feat-state", "03", "Isolated State", "Every user session instantly gets its own thread-local variables.");


// ===================================================================
// PAGE ROUTING
// ===================================================================

void renderHomePage(np::NovaBuilder& np) {
    
    np.onLoad([]() {
        counter = 0;
        quote = "Select a REST API method to test network connectivity.";
    });

    np.onClick("increment", []() { counter = counter + 1; });
    np.onClick("decrement", []() { counter = counter - 1; });

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

    // Render the Home Page Layout (12-Column Grid)
    np << R"(<div class="bento-grid">)";
    
    // ROW 1: Navigation
    np.renderComponent(navComp);
    
    // ROW 2: Interactive Components
    np.renderComponent(clockComp, 1000); 
    np.renderComponent(counterComp);
    np.renderComponent(speedFeat);
    
    // ROW 3: Extra Features & API
    np.renderComponent(spaFeat);
    np.renderComponent(stateFeat);
    np.renderComponent(apiComp);
    
    np << R"(</div>)";
}

class AboutComponent : public np::Component {
public:
    AboutComponent() : Component("about-comp") {}
    
    void render(np::NovaBuilder& np) override {
        np << R"(
        <div class="bento-card col-12 row-2" style="padding: 4rem; justify-content: center; align-items: center; text-align: center;">
            <h1 style="font-size: 3.5rem; margin-bottom: 1.5rem; color: #1d1d1f; letter-spacing: -1.5px; font-weight: 800;">About NovaCPP</h1>
            <p style="font-size: 1.25rem; line-height: 1.8; color: #86868b; max-width: 800px; margin-bottom: 1.5rem;">
                NovaCPP is an incredibly fast, highly optimized C++ web framework designed to bring the developer experience of React straight to the backend.
            </p>
            <p style="font-size: 1.25rem; line-height: 1.8; color: #86868b; max-width: 800px;">
                It completely eliminates the need for Node.js or JavaScript, allowing you to build real-time Single Page Applications (SPAs) purely in C++. Features include <strong style="color: #1d1d1f;">Surgical Routing</strong>, <strong style="color: #1d1d1f;">Automatic Live Polling</strong>, and <strong style="color: #1d1d1f;">Reactive State Management</strong>.
            </p>
        </div>
        )";
    }
};

AboutComponent aboutComp;

void renderAboutPage(np::NovaBuilder& np) {
    
    np << R"(<div class="bento-grid">)";
    np.renderComponent(navComp);
    np.renderComponent(aboutComp);
    np << R"(</div>)";
}
