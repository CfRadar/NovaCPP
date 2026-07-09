# 🌌 NovaCPP Framework

Welcome to **NovaCPP**, the fastest way to build beautiful, modern, interactive web applications using pure C++. 

NovaCPP gives you the developer experience of **React**, but completely inside a lightning-fast C++ server. It automatically handles isolated browser sessions, DOM rendering, state reactivity, and REST API fetching without you having to write a single line of backend routing logic.

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
That's it! The framework will automatically configure CMake, compile your C++ code, launch the live server in the background, and open `http://localhost:8080` in your browser.

---

## 🛠️ 2. The Core Syntax (Step-by-Step)

Everything you build in NovaCPP happens inside the `renderApp()` function in `src/App.cpp`. Let's explore the core features one by one.

### 🔹 Reactive State (`np::State`)
Just like React's `useState`, NovaCPP allows you to define reactive variables. However, because NovaCPP runs on a server, it uses **Automatic Session State**. This means that if 100 users connect to your app simultaneously, NovaCPP automatically isolates these variables so every single user gets their own private counter!

```cpp
// Define your state outside the render function
np::State<int> counter(0);
np::State<std::string> username("Guest");

// Inside your render function, access the value using .get()
int current_value = counter.get();
```

### 🔹 HTML Rendering (`np <<`)
You build your UI by streaming HTML strings into the `NovaBuilder` using the `<<` operator. The framework automatically parses this into the Virtual DOM and ships it to the browser.

```cpp
void renderApp(np::NovaBuilder& np) {
    np << "<h1>Welcome to NovaCPP</h1>";
    np << "<p>The counter is at: " + std::to_string(counter.get()) + "</p>";
}
```

### 🔹 Interactivity (`np.onClick`)
To make your app interactive, you can bind C++ lambdas to HTML buttons. When a user clicks the button in their browser, the C++ code executes instantly on the backend, and the UI dynamically updates without refreshing the page!

```cpp
void renderApp(np::NovaBuilder& np) {
    // 1. Define what happens when a button is clicked
    np.onClick("add_one", []() {
        counter = counter + 1; // Update your state
    });

    // 2. Bind the callback to your HTML using the 'nova-click' attribute
    np << "<button nova-click='add_one'>Click Me!</button>";
}
```

### 🔹 Lifecycle Hooks (`np.onLoad`)
Sometimes you need to reset variables or fetch initial data exactly when a user first opens your website (or when they hit refresh). Use `np.onLoad()` (which acts identically to React's `useEffect(..., [])`).

```cpp
void renderApp(np::NovaBuilder& np) {
    np.onLoad([]() {
        // This runs once per user exactly when they load the page
        counter = 0; 
    });
}
```

### 🔹 Native REST API Fetching (`np::fetch`)
Need to download JSON from the internet? Forget installing massive libraries like libcurl. NovaCPP has a blazing-fast, built-in HTTPS client powered by native Windows networking.

```cpp
void renderApp(np::NovaBuilder& np) {
    np.onClick("get_weather", []() {
        // Effortlessly download data from any public API over HTTPS
        std::string json = np::fetch("https://api.weather.gov/...");
        
        // Parse the JSON and update your state!
        weather_state = parse(json); 
    });
}
```

---

## 🎨 3. Styling Your App
To keep your C++ code pristine and readable, all of your CSS is automatically loaded from `render/styles.css`. Simply write standard CSS in that file, and your `App.cpp` components will beautifully inherit the styles.
