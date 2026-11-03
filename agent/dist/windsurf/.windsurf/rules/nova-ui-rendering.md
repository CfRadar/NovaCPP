# NovaCPP Rule: nova-ui-rendering

# NovaCPP UI Component Rendering & Bento Grid

<!-- symbols: Component, RenderContext, NovaBuilder -->

## When to use
Use this skill when creating modular UI components, writing HTML markup with stream operators (`np << "..."`), styling with the 12-column Bento Grid, or mounting views in `src/frontend/App.cpp`.

## Prerequisites
- A NovaCPP project with `src/frontend/App.cpp`.
- Understanding of the component lifetime invariant.

## Quick start
```cpp
#include "novacpp/nova.hpp"

class CounterCard : public np::Component {
public:
    CounterCard() : Component("counter-card") {}

    void render(np::RenderContext& np) override {
        np << "<div class='bento-card col-6'>"
           << "  <h3>Reactive Counter</h3>"
           << "  <button class='btn-primary' nova-click='inc' nova-target='counter-card'>+ Increment</button>"
           << "</div>";
    }
};

// Safe Static Lifetime
static CounterCard g_counterCard;

void renderHomePage(np::RenderContext& np) {
    np << "<div class='bento-grid'>";
    np.renderComponent(g_counterCard);
    np << "</div>";
}
```
Full compilable source available in [`assets/component_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-ui-rendering/assets/component_example.cpp).

## Steps
1. **Define a Component Class**:
   Inherit from `np::Component` and provide a unique ID in the constructor:
   ```cpp
   class MetricCard : public np::Component {
   public:
       MetricCard() : Component("metric-card") {}
       void render(np::RenderContext& np) override {
           np << "<div class='bento-card col-4'><h3>Active Users</h3><p>1,248</p></div>";
       }
   };
   ```
2. **Choose a Safe Lifetime Model**:
   - **Static / Global (Recommended for Shared UI)**:
     ```cpp
     static MetricCard g_metricCard;
     ```
   - **Heap-Allocated Shared Pointer (For Parameterized UI)**:
     ```cpp
     auto dynamicCard = std::make_shared<MetricCard>();
     np.renderComponent(dynamicCard); // Automatically registered to ServerContext
     ```
3. **Mount in the 12-Column Bento Grid**:
   Wrap components inside `<div class='bento-grid'>`:
   ```cpp
   np << "<div class='bento-grid'>";
   np.renderComponent(g_metricCard);
   np << "</div>";
   ```
   Apply grid column utility classes: `col-12` (full-width), `col-6` (half-width), `col-4` (third-width), `col-3` (quarter-width).
4. **Register Surgical Interactions**:
   In your page function, register the matching click or form actions:
   ```cpp
   np.onClick("refresh_metrics", []() { /* update state */ });
   ```

## Rules
- MUST assign a unique string ID to every component class constructor (`Component("unique-id")`).
- MUST follow the 12-column Bento Grid structure using `col-12`, `col-6`, `col-4`, etc.
- NEVER instantiate `np::Component` as a local stack variable inside route functions (`void renderHome(...) { MyComp comp; ... }` is strictly forbidden).
- NEVER build user-supplied content directly into raw HTML without escaping.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Runtime crash / `std::logic_error` | Component instantiated on the local stack inside route function | Use `static MyComp g_comp;` or `std::make_shared<MyComp>()` |
| Component layout misaligned in Bento Grid | Outer wrapper div not flattened by CSS | Ensure wrapper has `class="bento-grid"` which applies `display: contents` to children |
| Clicking button reloads entire page | Missing `nova-target` attribute on trigger button | Add `nova-target="comp-id"` matching the component's unique ID |

## Verify
Compile and view the rendered page in a browser:
```bash
nova.bat build
```
Verify that the component renders inside a `<div id="your-comp-id">` wrapper element.

## Go deeper
- [`references/component-lifetimes.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-ui-rendering/references/component-lifetimes.md): Complete guide to stack vs heap allocation, stack guards, and thread-local render builders.
