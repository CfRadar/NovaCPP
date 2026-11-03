# NovaCPP Component Lifetime & Memory Architecture Reference

## The Three Storage Models

| Allocation Strategy | Memory Segment | Lifetime | Recommended Use Case |
|---|---|---|---|
| **Global `static Comp g_comp;`** | Static Data Segment | Program Duration | Shared components, layout cards, navbars, sidebars |
| **`std::make_shared<Comp>()`** | Heap Segment | Reference Counted via `ServerContext` | Dynamic user profiles, parameterized rows, runtime widgets |
| **`Comp comp;` (Local Stack)** | Function Call Stack | Frame Duration | **FORBIDDEN (Throws `std::logic_error` Stack Guard)** |

---

## Why Local Stack Allocation Fails

When an HTTP request triggers `renderHomePage(np)`, local variables declared within `renderHomePage` reside on that thread's execution stack.
`NovaBuilder::renderComponent(Component& comp)` registers `&comp` in `ServerContext::components`.
As soon as `renderHomePage` returns, its stack frame is popped and the memory at `&comp` becomes invalid.

When a user subsequently clicks `<button nova-target="my-comp">`, the server receives `POST /nova/action/...` with `X-Nova-Target: my-comp`. The server attempts to look up `my-comp` in `components` and execute `comp->render()`. If `comp` was on the popped stack, this triggers an **access violation / use-after-free**.

NovaCPP includes an active Stack Guard `isAddressOnCurrentStack(&comp)` that prevents this failure mode by aborting with an informative `std::logic_error` immediately during development.
