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

class ApiFetchComponent : public np::Component {
public:
    ApiFetchComponent() : Component("api-comp") {}

    void render(np::NovaBuilder& np) override {
        np << R"(
        <div class="bento-card col-4">
            <div class="badge">REST API CLIENT</div>
            <div class="quote-box">
                <div class="quote-text">")" + quote.get() + R"("</div>
            </div>
            <button class="btn-primary" style="width: 100%;" nova-click="fetchData" nova-target="api-comp">Ask the Oracle</button>
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
ApiFetchComponent apiComp;

// Feature blocks (No Emojis)
FeatureBlock speedFeat("feat-speed", "01", "Blazing Fast", "Compiled C++ backend provides absolute millisecond response times.");
FeatureBlock spaFeat("feat-spa", "02", "SPA Routing", "Seamless page transitions without any browser reloads or flashing.");
FeatureBlock stateFeat("feat-state", "03", "Isolated State", "Every user session instantly gets its own thread-local variables.");


// ===================================================================
// PAGE ROUTING
// ===================================================================

void renderHomePage(np::NovaBuilder& np) {
    
    np.onLoad([]() {
        counter = 0;
        quote = "Click the button to receive wisdom.";
    });

    np.onClick("increment", []() { counter = counter + 1; });
    np.onClick("decrement", []() { counter = counter - 1; });

    np.onClick("fetchData", []() {
        quote = "Consulting the oracle...";
        std::string json = np::fetch("https://api.adviceslip.com/advice");
        std::string target = "\"advice\"";
        size_t start = json.find(target);
        if (start != std::string::npos) {
            start = json.find("\"", start + target.length()); 
            if (start != std::string::npos) {
                start++; 
                size_t end = json.find("\"", start); 
                if (end != std::string::npos) {
                    quote = json.substr(start, end - start);
                }
            }
        }
        if (quote.get() == "Consulting the oracle...") quote = "Error: The oracle is asleep.";
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
