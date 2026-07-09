# NovaCPP

NovaCPP is an ambitious C++ web development framework that allows you to write declarative, component-based UI code similar to React, but natively in C++.

## Project Structure
- `src/` - Write your application code here (`main.cpp`, `App.cpp`).
- `novacpp/` - The core framework headers containing the engine (Do not edit).
- `render/` - The compiled output HTML and assets.
- `build/` - CMake build artifacts.

## Syntax and Usage

### The `NovaBuilder` Class
Because `return` is a reserved keyword in C++, NovaCPP uses a builder pattern instead of a direct return statement like React's JSX. 

You pass HTML strings to the builder using the `np.render()` method.

**Example usage in `src/App.cpp`**:
```cpp
#include "../novacpp/html.hpp"

void renderApp(np::NovaBuilder& np) {
    // Adds a line of HTML to the final Virtual DOM output
    np.render("<h1>Welcome to NovaCPP</h1>");
    
    np.render("<div class='card'>");
    np.render("    <button>Click Me</button>");
    np.render("</div>");
}
```

### State and Re-rendering (Reactivity)
NovaCPP includes a reactive state system via the `np::State<T>` class. When a state variable changes, you can hook it up to automatically re-trigger a component render (just like React's `useState`).

**Example usage**:
```cpp
#include "../novacpp/html.hpp"
#include "../novacpp/state.hpp"

// Declare a reactive state variable
np::State<int> counter(0);

void renderApp(np::NovaBuilder& np) {
    np.render("<h1>Counter: " + std::to_string(counter.get()) + "</h1>");
}

// Somewhere in your code, when the variable changes:
// counter = 1; // This will trigger the registered re-render callback!
```

### Framework Initialization
In your `main.cpp`, the framework is instantiated and compiled into standard HTML:
```cpp
#include "../novacpp/html.hpp"

void renderApp(np::NovaBuilder& np);

int main() {
    // 1. Create the instance of our framework class
    np::NovaBuilder np;
    
    // 2. Call your App code to build the Virtual DOM / HTML list
    renderApp(np);
    
    // 3. Compile it and output to render/index.html
    np.compile("render/index.html");
    
    return 0;
}
```

## Commands
Use the included `nova.bat` wrapper script to easily build and run your project:

- **Build, Generate, and Open in Browser:**
  ```powershell
  .\nova build run
  ```
  *(Note: `.\nova run` also performs this exact same action)*

- **Build and Generate HTML Only:**
  ```powershell
  .\nova build
  ```

## How it Works under the hood
1. `main.cpp` instantiates a `np::NovaBuilder` object.
2. The builder is passed by reference to `renderApp()` (or any custom component function) which accumulates HTML elements into memory.
3. Finally, `np.compile("render/index.html")` is called. This wraps your raw elements in standard HTML boilerplate (`<html>`, `<body>`, etc.) and streams it to the `render` directory to be displayed by the browser.
