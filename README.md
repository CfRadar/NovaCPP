# 🌌 NovaCPP Framework

Welcome to **NovaCPP**, the holy grail of modern C++ web frameworks.

NovaCPP brings the developer experience of **React** directly into a lightning-fast C++ server. It provides true **Component-Based Architecture**, surgical DOM updates, automatic session management, and background live polling—all without requiring you to write a single line of JavaScript or backend routing logic.

---

## 🚀 1. Setup Instructions

To start building with NovaCPP on Windows, you only need standard C++ tools. You **do not** need Node.js, Python, or complex libraries like OpenSSL.

### Prerequisites
1. **CMake** (v3.10+) installed and added to your System PATH.
2. **C++ Compiler** (MSVC via Visual Studio is highly recommended).

### Running the App
1. Clone this repository.
2. Open a terminal (PowerShell) in the folder.
3. Run the automated build script:
```powershell
.\nova build run
```
That's it! The framework will automatically compile your C++ code, launch the live server in the background, and open `http://localhost:8080` in your browser.

---

## 🧩 2. Component Architecture (`np::Component`)

NovaCPP is built entirely around modular components. Instead of re-rendering the entire page when a variable changes (which causes screen flashing and wastes CPU), NovaCPP surgically updates the exact DOM element.

To create a component, inherit from `np::Component`:

```cpp
class CounterComponent : public np::Component {
public:
    // 1. Give your component a unique ID
    CounterComponent() : Component("counter-comp") {}
    
    // 2. Define its isolated HTML layout
    void render(np::NovaBuilder& np) override {
        np << "<div class='bento-card'>";
        np << "  <h1>" + std::to_string(counter.get()) + "</h1>";
        np << "  <button nova-click='increment' nova-target='counter-comp'>Increase</button>";
        np << "</div>";
    }
};
```

---

## ⚡ 3. The Core Syntax (Step-by-Step)

Everything you build in NovaCPP happens inside the `renderApp()` function in `src/App.cpp`. Let's explore the core features.

### 🔹 Reactive State (`np::State`)
Just like React's `useState`, NovaCPP allows you to define reactive variables. Because NovaCPP runs on a server, it uses **Automatic Session State**. If 100 users connect to your app simultaneously, NovaCPP automatically isolates these variables so every single user gets their own private counter!

```cpp
// Define your state globally
np::State<int> counter(0);
np::State<std::string> username("Guest");

// Access the value inside your components using .get()
int current_value = counter.get();
```

### 🔹 HTML Rendering (`np <<`)
You build your UI by streaming raw HTML strings into the `NovaBuilder` using the `<<` operator. The framework automatically parses this into the Virtual DOM and ships it to the browser.

```cpp
void render(np::NovaBuilder& np) override {
    np << "<h1>Welcome to NovaCPP</h1>";
}
```

### 🔹 Interactivity (`np.onClick`)
To make your app interactive, you can bind C++ lambdas to HTML buttons. When a user clicks the button in their browser, the C++ code executes instantly on the backend.

```cpp
void renderApp(np::NovaBuilder& np) {
    // 1. Define what happens when a button is clicked
    np.onClick("add_one", []() {
        counter = counter + 1; // Update your state
    });
}
```

### 🔹 Surgical DOM Updates (`nova-target`)
This is the magic of NovaCPP. If you bind a button to an action, you can tell the JavaScript engine to ONLY re-render a specific component using the `nova-target` attribute.

```html
<!-- This button tells the server to ONLY update the HTML inside 'counter-comp' -->
<button nova-click="add_one" nova-target="counter-comp">Click Me!</button>
```

### 🔹 Lifecycle Hooks (`np.onLoad`)
Sometimes you need to reset variables or fetch initial data exactly when a user first opens your website. Use `np.onLoad()` (which acts identically to React's `useEffect(..., [])`).

```cpp
void renderApp(np::NovaBuilder& np) {
    np.onLoad([]() {
        // This runs once per user exactly when they load the page
        counter = 0; 
    });
}
```

---

## ⏱️ 4. Automatic Live Polling

Building a real-time dashboard or a ticking clock? You do not need WebSockets. NovaCPP supports automatic background Live Polling!

When you render a component in `renderApp`, simply pass an optional integer representing milliseconds. 

```cpp
ClockComponent clockComp;

void renderApp(np::NovaBuilder& np) {
    // This tells the frontend to automatically refresh this exact component every 1000 milliseconds!
    np.renderComponent(clockComp, 1000); 
}
```
The browser will silently ping the C++ server in the background every second, execute the component's logic (like fetching the system time), and magically swap the component's HTML with the latest data in real-time.

---

## 🌐 5. Native REST API Fetching (`np::fetch`)

Need to download JSON from the internet? Forget installing massive libraries like libcurl. NovaCPP has a blazing-fast, built-in HTTPS client powered by native Windows networking.

```cpp
void renderApp(np::NovaBuilder& np) {
    np.onClick("get_weather", []() {
        // Effortlessly download data from any public API over HTTPS
        std::string json = np::fetch("https://api.weather.gov/...");
        
        // Parse the JSON and update your state!
    });
}
```

---

## 🎨 6. Styling Your App
To keep your C++ code pristine and readable, all of your CSS is automatically loaded from `render/styles.css`. Simply write standard CSS in that file, and your components will beautifully inherit the styles. The boilerplate includes a stunning, premium **Bento Box Architecture** out of the box!
