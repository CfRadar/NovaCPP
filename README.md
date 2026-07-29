# NovaCPP

<p align="left">
  <a href="https://github.com/CfRadar/NovaCPP/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="NovaCPP license"></a>
  <a href="#"><img src="https://img.shields.io/badge/build-passing-brightgreen.svg" alt="Build Status"></a>
  <a href="#"><img src="https://img.shields.io/badge/language-C%2B%2B17-blue.svg" alt="C++ version"></a>
  <a href="https://github.com/CfRadar/NovaCPP/pulls"><img src="https://img.shields.io/badge/PRs-welcome-brightgreen.svg" alt="PRs Welcome"></a>
</p>

NovaCPP is a C++ web framework for building responsive user interfaces.

* **Declarative:** NovaCPP makes it painless to create interactive UIs. Design simple views for each state in your application, and NovaCPP will efficiently update and render just the right components when your data changes. Declarative views make your code more predictable, simpler to understand, and easier to debug.
* **Component-Based:** Build encapsulated components that manage their own state, then compose them to make complex UIs. Since component logic is written in C++ instead of templates, you can easily pass rich data through your app and keep the state out of the DOM.
* **Full-Stack Monolith (No-API):** Write your UI layouts, business logic, and database integrations in a single, latency-free C++ executable. Because your frontend UI and your backend logic share the same memory space, there is no need to write REST endpoints or manage complex JSON APIs.

[Learn how to use NovaCPP in your project](#documentation).

---

## Installation

NovaCPP has been designed for gradual adoption from the start, and you can use as little or as much NovaCPP as you need:

* **Quick Start:** Use the `create-novacpp` CLI tool to instantly bootstrap a new project. Open your terminal and run:
  ```bash
  npx create-novacpp@latest my-app
  cd my-app
  npm run dev
  ```
  This command will automatically set up the project, compile the C++ source code, boot the server, and open `http://localhost:8080` in your default browser.
* **Add NovaCPP to an Existing Project:** Integrate the `novacpp/` directory directly into your C++ CMake project as a header-only library.

---

## Documentation

You can find the NovaCPP documentation directly in this repository.

### Table of Contents
- [Core Architecture](#core-architecture)
- [API Reference](#api-reference)
  - [np::NovaBuilder](#npnovabuilder)
  - [np::Component](#npcomponent)
  - [np::State](#npstatet)
  - [np::fetch](#npfetch)
- [Frontend HTML Directives](#frontend-html-directives)
- [Styling & Grid System](#styling--grid-system)
- [Cloud Deployment](#cloud-deployment)

### Core Architecture

NovaCPP operates by generating HTML on the server and surgically swapping DOM elements on the client without ever triggering a full browser refresh.

#### The Full-Stack Monolith
Because your frontend UI and your backend logic are compiled into the exact same native C++ `.exe` binary, they share the same memory space. Your UI components can read backend variables, verify authentication, or query databases at the speed of C++ RAM (nanoseconds).

#### The "No-API" Paradigm
In traditional web development (like React + Node.js), developers build JSON REST API routes (e.g., `/api/checkout`) because the frontend and backend are physically disconnected. 
In NovaCPP, the frontend IS the backend. Your UI can trigger backend functions directly. For example:
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

---

## Examples

Here is a simple interactive Counter example to get you started:

```cpp
#include "../novacpp/html.hpp"
#include "../novacpp/state.hpp"

// 1. Declare a reactive state variable
np::State<int> counter(0);

void renderApp(np::NovaBuilder& np) {
    // 2. Register C++ callbacks for button clicks
    np.onClick("increment", []() {
        counter = counter.get() + 1; // Pure C++ logic!
    });

    np.onClick("decrement", []() {
        counter = counter.get() - 1; 
    });

    // 3. Render the UI
    np << "<div class='card'>"
       << "    <h1>Counter: " + std::to_string(counter.get()) + "</h1>"
       << "    <button nova-click='decrement'>- Decrement</button>"
       << "    <button nova-click='increment'>+ Increment</button>"
       << "</div>";
}
```

This example will render a counter card and bind the HTML buttons to C++ lambdas, which reactively update the state and swap the DOM elements in real-time.

---

### API Reference

#### `np::NovaBuilder`
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

#### `np::Component`
The base class for modular UI elements. Inherit from this class and override the `render` method:
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

#### `np::State<T>`
Thread-local reactive state variables. NovaCPP uses **Automatic Session State**. If 1,000 users connect simultaneously, the framework inherently isolates these variables so every user receives their own private memory space.
```cpp
np::State<int> counter(0);

// Access via .get()
int val = counter.get();

// Mutate state directly via assignment
counter = val + 1;
```

#### `np::fetch`
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

### Frontend HTML Directives

While you write your UI in C++, the HTML you generate must use specific NovaCPP attributes to trigger the JavaScript engine:
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

### Styling & Grid System

The framework ships with a mathematically precise, ultra-modern **12-column Bento Grid** located in `render/styles.css`.

To layout your components, apply column span classes (`col-12`, `col-6`, `col-4`) to lock them into the responsive grid:
```html
<!-- Occupies exactly 1/3rd of the screen width -->
<div class="bento-card col-4">
    <h3>Card Title</h3>
</div>
```

---

### Cloud Deployment

NovaCPP is engineered for modern cloud infrastructure (Railway, Render, Fly.io) via Docker containerization.

#### Dual Network Engine Architecture
1. **Local Windows Development**: The framework compiles using ultra-fast `WinHTTP` Native Networking.
2. **Linux Cloud Deployment**: When the `Dockerfile` builds your project on a Linux server, the C++ preprocessor seamlessly injects a secure, OpenSSL-backed network fallback without requiring any changes to your code.

#### Deployment Steps
Because NovaCPP is a persistent, stateful C++ backend, it **cannot** be hosted on serverless platforms like Vercel.
1. Push your repository to **GitHub**.
2. Log into a container-hosting platform like [Railway.app](https://railway.app/).
3. Select **"Deploy from GitHub repo"**.
4. Navigate to your Service Settings > Networking and click **"Generate Domain"** to expose your public URL.

---

## Contributing

The main purpose of this repository is to continue evolving NovaCPP core, making it faster and easier to use. Development of NovaCPP happens in the open on GitHub, and we are grateful to the community for contributing bugfixes and improvements.

### Code of Conduct
We expect all project contributors and participants to adhere to our Code of Conduct. Please read the full text to understand what actions will and will not be tolerated.

### Contributing Guide
Read our contributing guide to learn about our development process, how to propose bugfixes and improvements, and how to build and test your changes to NovaCPP.

---

## License

NovaCPP is [MIT licensed](LICENSE).
