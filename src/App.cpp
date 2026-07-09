#include "../novacpp/html.hpp"
#include "../novacpp/state.hpp"

// Our reactive C++ state variable
np::State<int> counter(0);

void renderApp(np::NovaBuilder& np) {
    // 1. Define C++ callbacks for button clicks!
    np.onClick("increment", []() {
        counter = counter + 1;
    });

    np.onClick("decrement", []() {
        counter = counter - 1;
    });

    // 2. Render the UI and bind buttons to our callbacks
    np << "<div class='card' style='text-align: center; font-family: sans-serif;'>"
       << "    <h1>Counter: " + std::to_string(counter.get()) + "</h1>"
       << "    <br>"
       // The nova-click attribute matches the name we registered in onClick()
       << "    <button nova-click='decrement' style='padding: 10px; margin: 5px;'>- Decrement</button>"
       << "    <button nova-click='increment' style='padding: 10px; margin: 5px;'>+ Increment</button>"
       << "</div>";
}
