#include "../novacpp/html.hpp"
#include "../novacpp/state.hpp"
#include "../novacpp/fetch.hpp"

// -------------------------------------------------------------------
// 1. DECLARE REACTIVE STATE
// These variables automatically isolate themselves for every connected
// browser session, behaving exactly like React's `useState`.
// -------------------------------------------------------------------
np::State<int> counter(0);
np::State<std::string> quote("Click the button to receive wisdom.");

// -------------------------------------------------------------------
// 2. THE MAIN RENDER COMPONENT
// This function describes your UI. It runs once on page load, and 
// automatically re-runs whenever a button is clicked.
// -------------------------------------------------------------------
void renderApp(np::NovaBuilder& np) {
    
    // np.onLoad acts exactly like React's useEffect(..., [])
    // It runs once every time the user refreshes or loads the page.
    np.onLoad([]() {
        counter = 0;
        quote = "Click the button to receive wisdom.";
    });

    // np.onClick binds a C++ lambda function to an HTML button.
    np.onClick("increment", []() { counter = counter + 1; });
    np.onClick("decrement", []() { counter = counter - 1; });

    // You can even fetch REST APIs effortlessly over HTTPS using np::fetch!
    np.onClick("fetchData", []() {
        quote = "Consulting the oracle...";
        std::string json = np::fetch("https://api.adviceslip.com/advice");
        
        // Find the quote string inside the JSON
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
        
        if (quote.get() == "Consulting the oracle...") {
            quote = "Error: The oracle is asleep.";
        }
    });

    // -------------------------------------------------------------------
    // 3. RENDER THE VIRTUAL DOM
    // Use the `<<` operator to chain HTML components. Notice how we 
    // connect the buttons to the C++ lambdas using `nova-click="..."`.
    // (All styling is imported automatically from render/styles.css)
    // -------------------------------------------------------------------
    np << R"(
    <div class="container">
        <!-- Counter Component -->
        <div class="glass-card">
            <div class="badge">State Hook</div>
            <div class="counter-val">)" + std::to_string(counter.get()) + R"(</div>
            <div class="btn-group">
                <button class="btn-secondary" nova-click="decrement">Decrease</button>
                <button class="btn-primary" nova-click="increment">Increase</button>
            </div>
        </div>

        <!-- API Fetch Component -->
        <div class="glass-card">
            <div class="badge">REST API Client</div>
            <div class="quote-text">")" + quote.get() + R"("</div>
            <button class="btn-primary" style="width: 100%;" nova-click="fetchData">Ask the Oracle</button>
        </div>
    </div>
    )";
}
