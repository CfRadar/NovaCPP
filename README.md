# NovaCPP 🚀

NovaCPP is an ambitious, modern C++ web development framework. It brings the declarative, component-based UI experience of popular JavaScript frameworks (like React) directly into native C++. 

NovaCPP features a blazing-fast **Backend-Driven Live Web Architecture**. Your C++ application acts as an active backend server, automatically synchronizing its state with the browser in real-time without you ever having to write complex JavaScript or deal with WebAssembly toolchains.

## ✨ Features

- **Declarative UI Syntax**: Write HTML components seamlessly using a clean, chained builder syntax (`operator<<`).
- **Reactive State Management**: Built-in state wrappers (`np::State<T>`) that automatically manage values and UI synchronization.
- **Backend-Driven Interactivity**: Bind HTML button clicks directly to native C++ lambdas. NovaCPP handles the background network requests and DOM swapping invisibly.
- **Live C++ Web Server**: Includes an embedded, lightning-fast HTTP server that dynamically serves routes and updates.
- **Zero Configuration Setup**: The included wrapper script automatically manages CMake initialization, compilation, and execution.

## 📋 Prerequisites

To develop with NovaCPP, ensure you have the following installed on your system:
- **C++ Compiler**: A modern C++17 compatible compiler (e.g., MSVC, GCC, Clang).
- **CMake**: Version 3.10 or higher.
- *(For Windows Users)*: Visual Studio Build Tools (Desktop development with C++ workload).

## 🚀 Getting Started

Getting started is incredibly easy. Simply clone the repository and run the included script!

1. Clone the repository:
   ```bash
   git clone https://github.com/CfRadar/NovaCPP.git
   cd NovaCPP
   ```

2. Build and run the Live Server:
   ```powershell
   .\nova build run
   ```
   *This single command automatically configures CMake (if it's a fresh clone), compiles the C++ source code, launches the live HTTP server, and instantly opens your default browser!*

## 📖 Syntax and Usage

### The `NovaBuilder` Class
NovaCPP uses a builder pattern for rendering UI. You can seamlessly chain HTML elements together using the elegant `<<` operator, or utilize C++ Raw String Literals (`R"(...)"`) for massive, multi-line HTML blocks without quotes.

**Example component (`src/App.cpp`)**:
```cpp
#include "../novacpp/html.hpp"
#include "../novacpp/state.hpp"

// 1. Declare a reactive state variable
np::State<int> counter(0);

void renderApp(np::NovaBuilder& np) {
    // 2. Register C++ callbacks for button clicks
    np.onClick("increment", []() {
        counter = counter + 1; // Pure C++ logic!
    });

    np.onClick("decrement", []() {
        counter = counter - 1; 
    });

    // 3. Render the Virtual DOM
    np << "<div class='card'>"
       << "    <h1>Counter: " + std::to_string(counter.get()) + "</h1>"
       // Use the nova-click attribute to bind the HTML button directly to the C++ callback
       << "    <button nova-click='decrement'>- Decrement</button>"
       << "    <button nova-click='increment'>+ Increment</button>"
       << "</div>";
}
```

### Framework Initialization
In your `main.cpp`, the framework is instantiated and the server is launched:
```cpp
#include "../novacpp/html.hpp"

void renderApp(np::NovaBuilder& np);

int main() {
    // 1. Create the instance of the framework
    np::NovaBuilder np;
    
    // 2. Start the Live Server on port 8080
    // We pass renderApp so the server can automatically re-render 
    // the UI whenever a button triggers a state change.
    np.listen(8080, renderApp);
    
    return 0;
}
```

## 🛠️ Project Structure
- `src/` - Write your application code here (`main.cpp`, `App.cpp`).
- `novacpp/` - The core framework headers containing the engine (Do not edit).
- `render/` - Static assets and the client-side JavaScript bridge (`nova.js`).
- `build/` - CMake build artifacts (Generated automatically).
