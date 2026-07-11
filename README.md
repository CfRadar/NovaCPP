# 🌌 NovaCPP Framework

NovaCPP is an enterprise-grade, high-performance C++ web framework designed to bring the developer experience of modern frontend libraries (like React) directly into a native C++ backend.

By leveraging a **Backend-For-Frontend (BFF)** and **Full-Stack Monolith** architecture, NovaCPP eliminates the need for Node.js, JavaScript, and complex JSON REST APIs. You write your UI layouts, business logic, and database integrations in a single, latency-free C++ executable.

---

## 🏗️ 1. Core Architecture

NovaCPP operates by generating HTML on the server and surgically swapping DOM elements on the client without ever triggering a full browser refresh. 

### The Full-Stack Monolith
Because your frontend UI and your backend logic are compiled into the exact same native C++ `.exe` binary, they share the same memory space. Your UI components can read backend variables, verify authentication, or query databases at the speed of C++ RAM (nanoseconds). 

### The "No-API" Paradigm (Why you don't need REST endpoints)
In traditional web development (like React + Node.js), developers are forced to build JSON REST API routes (e.g., `/api/checkout`) because the frontend and backend are physically disconnected. The frontend has to hit a network endpoint just to talk to the backend.

**In NovaCPP, the frontend IS the backend.** Because your UI and backend logic live in the exact same C++ program, your UI can trigger backend functions directly. If you want to integrate Stripe or a database, you just execute that logic inside a button click:
```cpp
np.onClick("checkout", []() {
    // 1. Process payment via Stripe (Backend)
    bool success = Stripe::chargeCustomer(1500);

    // 2. Save to Postgres (Backend)
    if (success) Database::saveOrder();

    // 3. Update the UI instantly!
    checkoutState = "Payment Successful!";
});
```
The C++ server executes the Stripe logic, queries the database, and updates the HTML instantly—all without you ever needing to write a single `/api/...` JSON route!

### Recommended Directory Structure
As your application scales, it is highly recommended to split your project into frontend UI components and pure backend services:

```text
/src
  /frontend
     - App.cpp       (UI layouts, np::Component definitions, Routes)
  /backend
     - Database.cpp  (SQL connections, Redis caching)
     - Auth.cpp      (Business logic, password hashing)
  - main.cpp         (Server entry point & boot sequence)
```

---

## 🚀 2. Getting Started

### Prerequisites
1. **CMake** (v3.10+) added to your System PATH.
2. **C++ Compiler** (MSVC via Visual Studio on Windows, or GCC/Clang on Linux).

### Running Locally
Use the integrated build script to compile and run the Live Server:
```powershell
.\nova build run
```
The server will boot and open `http://localhost:8080` in your default browser.

---

## 📚 3. API Reference & Syntax

### `np::NovaBuilder`
The core framework instance responsible for routing, event handling, and HTML stream building.

* **`route(std::string path, std::function<void(np::NovaBuilder&)> callback)`**
  Registers a Single Page Application (SPA) route.
* **`listen(int port)`**
  Boots the native HTTP server on the specified port.
* **`onClick(std::string actionName, std::function<void()> callback)`**
  Binds a C++ lambda function to a frontend `nova-click` event.
* **`onLoad(std::function<void()> callback)`**
  Executes logic exactly when a user navigates to the route (similar to React's `useEffect`).
* **`renderComponent(np::Component& comp, int pollIntervalMs = 0)`**
  Renders a modular component into the layout. If `pollIntervalMs` is provided, the browser will automatically fetch real-time updates for this component in the background (Live Polling).
* **`operator<<`**
  Streams raw HTML strings directly into the Virtual DOM.

### `np::Component`
The base class for modular UI elements. Inherit from this class and override the `render` method.

```cpp
class CounterComponent : public np::Component {
public:
    CounterComponent() : Component("counter-comp") {} // Assign a unique ID
    
    void render(np::NovaBuilder& np) override {
        np << "<div class='bento-card col-4'>";
        np << "  <button nova-click='increment' nova-target='counter-comp'>Increase</button>";
        np << "</div>";
    }
};
```

### `np::State<T>`
Thread-local reactive state variables. NovaCPP uses **Automatic Session State**. If 1,000 users connect simultaneously, the framework inherently isolates these variables so every user receives their own private memory space.

```cpp
np::State<int> counter(0);

// Access via .get()
int val = counter.get();

// Mutate state directly via assignment
counter = val + 1;
```

### `np::fetch`
A built-in, native HTTP client for interacting with external REST APIs. It supports all standard HTTP methods (`GET`, `POST`, `PUT`, `DELETE`).

```cpp
// GET Request
std::string res = np::fetch("https://api.example.com/data");

// POST Request with Body and Headers
std::string body = R"({"name": "NovaCPP"})";
std::string headers = "Content-Type: application/json\r\n";
std::string res = np::fetch("https://api.example.com/create", "POST", body, headers);
```

---

## 🧩 4. Frontend HTML Directives

While you write your UI in C++, the HTML you generate must use specific NovaCPP attributes to trigger the JavaScript engine.

* **`nova-link`**: Add this to any `<a>` tag to enable instantaneous SPA routing without a page reload.
  ```html
  <a href="/about" nova-link>About Us</a>
  ```
* **`nova-click="actionName"`**: Binds an HTML element to a C++ `np.onClick` lambda.
* **`nova-target="componentId"`**: Instructs the frontend engine to surgically replace *only* the specified component when the click event returns, preventing screen flashes.
  ```html
  <button nova-click="add_one" nova-target="counter-comp">Click</button>
  ```

---

## 🎨 5. Styling & Grid System

The framework ships with a mathematically precise, ultra-modern **12-column Bento Grid** located in `render/styles.css`.

To layout your components, apply column span classes (`col-12`, `col-6`, `col-4`) to lock them into the responsive grid:
```html
<!-- Occupies exactly 1/3rd of the screen width -->
<div class="bento-card col-4">
    <h3>Card Title</h3>
</div>
```

---

## ☁️ 6. Cloud Deployment

NovaCPP is engineered for modern cloud infrastructure (Railway, Render, Fly.io) via Docker containerization. 

### Dual Network Engine Architecture
1. **Local Windows Development**: The framework compiles using ultra-fast `WinHTTP` Native Networking.
2. **Linux Cloud Deployment**: When the `Dockerfile` builds your project on a Linux server, the C++ preprocessor seamlessly injects a secure, OpenSSL-backed network fallback without requiring any changes to your code.

### Deployment Steps
Because NovaCPP is a persistent, stateful C++ backend, it **cannot** be hosted on serverless platforms like Vercel. 

1. Push your repository to **GitHub**.
2. Log into a container-hosting platform like [Railway.app](https://railway.app/).
3. Select **"Deploy from GitHub repo"**.
4. Navigate to your Service Settings > Networking and click **"Generate Domain"** to expose your public URL.

The platform will automatically detect the provided `Dockerfile`, compile the C++ binary inside a tiny Ubuntu environment, and host your Full-Stack Monolith live on the internet!
