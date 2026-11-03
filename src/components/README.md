# NovaCPP Modular UI Components

This directory is intended for modular, reusable UI components inheriting from `np::Component`.

## Component Lifecycle & Lifetime Safety Rule

NovaCPP stores pointers to components for surgical DOM patch updates. Always ensure components have a stable lifetime:
1. **Global or namespace scope**: `static MyComponent g_myComp;`
2. **Static local scope**: Inside helper functions: `static MyComponent s_myComp;`
3. **Heap allocation with shared ownership**: `np.renderComponent(std::make_shared<MyComponent>());`

**Never instantiate components as local stack variables inside route handler functions.** Local stack allocation will trigger a lifetime violation exception.

## Example Component Implementation

```cpp
#pragma once
#include "../../novacpp/html.hpp"
#include "../../novacpp/state.hpp"

class ProfileCard : public np::Component {
public:
    ProfileCard() : Component("profile-card") {}

    void render(np::RenderContext& np) override {
        np << R"html(
        <div class="bento-card col-6">
            <h3>Developer Profile</h3>
            <p>Welcome to NovaCPP!</p>
        </div>
        )html";
    }
};
```
