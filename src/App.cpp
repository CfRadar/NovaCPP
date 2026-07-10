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

class ClockComponent : public np::Component {
public:
    // Pass a unique ID for this component to the base class
    ClockComponent() : Component("clock-comp") {}
    
    // The render function defines the HTML layout for this component
    void render(np::NovaBuilder& np) override {
        // Get the current server time using standard C++ logic
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", ltm);
        std::string timeStr(buffer);

        // Stream the HTML layout using CSS classes from styles.css
        np << R"(
        <div class="bento-card">
            <div class="badge badge-live">Live Polling</div>
            <div class="counter-val">)" + timeStr + R"(</div>
            <div class="quote-text" style="margin-bottom: 0;">This C++ clock updates automatically!</div>
        </div>
        )";
    }
};

class CounterComponent : public np::Component {
public:
    CounterComponent() : Component("counter-comp") {}
    
    void render(np::NovaBuilder& np) override {
        np << R"(
        <div class="bento-card">
            <div class="badge">State Hook</div>
            <div class="counter-val">)" + std::to_string(counter.get()) + R"(</div>
            <div class="btn-group">
                <!-- 
                  Notice the 'nova-target' attribute! 
                  It tells the JavaScript engine to ONLY re-render this 
                  specific component, preventing the whole screen from flashing! 
                -->
                <button class="btn-secondary" nova-click="decrement" nova-target="counter-comp">Decrease</button>
                <button class="btn-primary" nova-click="increment" nova-target="counter-comp">Increase</button>
            </div>
        </div>
        )";
    }
};

class ApiFetchComponent : public np::Component {
public:
    ApiFetchComponent() : Component("api-comp") {}

    void render(np::NovaBuilder& np) override {
        // Note the "span-2-col" class makes this card stretch across the bento grid
        np << R"(
        <div class="bento-card span-2-col">
            <div class="badge">REST API Client</div>
            <div class="quote-text">")" + quote.get() + R"("</div>
            <button class="btn-primary" style="width: 100%; max-width: 300px;" nova-click="fetchData" nova-target="api-comp">Ask the Oracle</button>
        </div>
        )";
    }
};

// Instantiate our components globally
ClockComponent clockComp;
CounterComponent counterComp;
ApiFetchComponent apiComp;


// ===================================================================
// THE MAIN APP
// This is the entry point for your UI layout and event listeners.
// ===================================================================
void renderApp(np::NovaBuilder& np) {
    
    // np.onLoad runs once every time the user refreshes the page
    np.onLoad([]() {
        counter = 0;
        quote = "Click the button to receive wisdom.";
    });

    // np.onClick binds C++ backend logic to your HTML buttons
    np.onClick("increment", []() { counter = counter + 1; });
    np.onClick("decrement", []() { counter = counter - 1; });

    np.onClick("fetchData", []() {
        quote = "Consulting the oracle...";
        
        // Fetch data natively in C++ using WinHTTP
        std::string json = np::fetch("https://api.adviceslip.com/advice");
        
        // Parse the raw JSON string to extract the quote
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

    // ---------------------------------------------------------------
    // RENDER THE BENTO GRID LAYOUT
    // ---------------------------------------------------------------
    np << R"(<div class="bento-grid">)";
    
    // Render the ClockComponent and tell it to automatically poll every 1000ms!
    np.renderComponent(clockComp, 1000); 
    
    // Render the interactive components
    np.renderComponent(counterComp);
    np.renderComponent(apiComp);
    
    np << R"(</div>)";
}
