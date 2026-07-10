# 🌌 NovaCPP Framework

Welcome to **NovaCPP**, the holy grail of modern C++ web frameworks.

NovaCPP brings the developer experience of **React** directly into a lightning-fast C++ server. It provides true **Component-Based Architecture**, surgical DOM updates, Single Page Application (SPA) routing, automatic session management, and background live polling—all without requiring you to write a single line of JavaScript or backend routing logic.

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

## 🗺️ 2. SPA Routing (`np.route`)

NovaCPP is a fully-featured **Single Page Application (SPA)** framework. You can define multiple pages (routes) and navigate between them instantly, without the browser ever doing a hard reload.

In `src/main.cpp`, register your page render functions:
```cpp
int main() {
    np::NovaBuilder np;
    
    np.route("/", renderHomePage);
    np.route("/about", renderAboutPage);
    
    np.listen(8080);
}
```

To create a lightning-fast navigation link in your UI, simply add the `nova-link` attribute:
```html
<a href="/about" nova-link>About Us</a>
```
The JavaScript engine intercepts the click, pushes the URL to your browser history, and magically swaps out the page's HTML in milliseconds!

---

## 🧩 3. Component Architecture (`np::Component`)

NovaCPP is built entirely around modular components. Instead of re-rendering the entire page when a variable changes (which causes screen flashing and wastes CPU), NovaCPP surgically updates the exact DOM element.

To create a component, inherit from `np::Component`:

```cpp
class CounterComponent : public np::Component {
public:
    // 1. Give your component a unique ID
    CounterComponent() : Component("counter-comp") {}
    
    // 2. Define its isolated HTML layout
    void render(np::NovaBuilder& np) override {
        np << "<div class='bento-card col-4'>";
        np << "  <h1>" + std::to_string(counter.get()) + "</h1>";
        np << "  <button nova-click='increment' nova-target='counter-comp'>Increase</button>";
        np << "</div>";
    }
};
```

---

## ⚡ 4. The Core Syntax (Step-by-Step)

Everything you build in NovaCPP happens inside your route render functions (e.g., `renderHomePage`) in `src/App.cpp`. Let's explore the core features.

### 🔹 Reactive State (`np::State`)
Just like React's `useState`, NovaCPP allows you to define reactive variables. Because NovaCPP runs on a server, it uses **Automatic Session State**. If 100 users connect to your app simultaneously, NovaCPP automatically isolates these variables so every single user gets their own private counter!

```cpp
// Define your state globally
np::State<int> counter(0);

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
void renderHomePage(np::NovaBuilder& np) {
    np.onClick("add_one", []() {
        counter = counter + 1; // Update your state
    });
}
```

### 🔹 Surgical DOM Updates (`nova-target`)
This is the magic of NovaCPP. If you bind a button to an action, you can tell the JavaScript engine to ONLY re-render a specific component using the `nova-target` attribute.

```html
<button nova-click="add_one" nova-target="counter-comp">Click Me!</button>
```

### 🔹 Lifecycle Hooks (`np.onLoad`)
Sometimes you need to reset variables or fetch initial data exactly when a user first opens a specific page. Use `np.onLoad()` (which acts identically to React's `useEffect(..., [])`).

```cpp
void renderHomePage(np::NovaBuilder& np) {
    np.onLoad([]() {
        counter = 0; 
    });
}
```

---

## ⏱️ 5. Automatic Live Polling

Building a real-time dashboard or a ticking clock? You do not need WebSockets. NovaCPP supports automatic background Live Polling!

When you render a component in your layout, simply pass an optional integer representing milliseconds. 

```cpp
void renderHomePage(np::NovaBuilder& np) {
    // Automatically refresh this exact component every 1000 milliseconds!
    np.renderComponent(clockComp, 1000); 
}
```
The browser will silently ping the C++ server in the background every second, execute the component's logic, and swap the HTML in real-time.

---

## 🌐 6. Native Full REST API Client (`np::fetch`)

Forget installing massive, bloated libraries like `libcurl`. NovaCPP features a blazing-fast, built-in HTTPS client powered by native Windows networking. 

You are not limited to just fetching data. Your C++ backend can perform full CRUD operations (`GET`, `POST`, `PUT`, `DELETE`), allowing you to talk to databases, Stripe, OpenAI, and any other REST API natively!

```cpp
void renderHomePage(np::NovaBuilder& np) {
    
    // 1. Fetching Data (GET)
    np.onClick("get_data", []() {
        std::string json = np::fetch("https://api.example.com/users");
    });

    // 2. Creating Data (POST)
    np.onClick("create_user", []() {
        std::string body = R"({"name": "NovaCPP", "role": "admin"})";
        std::string headers = "Content-Type: application/json\r\n";
        
        // Pass the method, body, and custom headers directly into fetch!
        std::string response = np::fetch("https://api.example.com/users", "POST", body, headers);
    });

    // 3. Updating Data (PUT)
    np.onClick("update_user", []() {
        std::string body = R"({"name": "NovaCPP v2"})";
        std::string headers = "Content-Type: application/json\r\n";
        std::string response = np::fetch("https://api.example.com/users/1", "PUT", body, headers);
    });

    // 4. Deleting Data (DELETE)
    np.onClick("delete_user", []() {
        std::string response = np::fetch("https://api.example.com/users/1", "DELETE");
    });
}
```

---

## 🎨 7. The 12-Column Bento Grid Styling
To keep your C++ code pristine and professional, the framework includes a mathematically perfect, ultra-modern **12-column Bento Grid** out of the box in `styles.css`.

When defining components, simply use column span classes (`col-12`, `col-6`, `col-4`) to lock them perfectly into the grid:
```html
<!-- This card will take up exactly 1/3rd of the screen width -->
<div class="bento-card col-4">
    Content...
</div>
```

---

## ☁️ 8. Cloud Deployment (Docker & Railway/Render)

NovaCPP is a fully cross-platform codebase built to be deployed on modern cloud architecture. 

Because NovaCPP is a high-performance, persistent C++ backend (and not a serverless function), **it cannot be hosted on Vercel**. Instead, it includes a highly-optimized, multi-stage `Dockerfile` designed for deployment on **Railway**, **Render**, or **Fly.io**.

The framework features a **Dual Network Engine**:
1. When you run `nova dev` on **Windows**, it automatically compiles using ultra-fast `WinHTTP` Native Networking.
2. When the `Dockerfile` builds your project on a **Linux** server, the preprocessor seamlessly injects a secure OpenSSL-backed network fallback without you changing a single line of your application code!

### How to Deploy
1. Push your NovaCPP folder to a **GitHub Repository**.
2. Log into [Railway.app](https://railway.app/) or [Render.com](https://render.com/).
3. Select **"Deploy from GitHub repo"** and choose your repository.

The platform will automatically detect the `Dockerfile`, spin up a Linux environment, compile your C++ binary, and host your framework live on the internet!
