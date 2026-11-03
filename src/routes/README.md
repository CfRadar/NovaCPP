# NovaCPP Page Routes

This directory is intended for page render functions and API route definitions.

## Recommended Structure

- `HomeRoute.cpp`: Renders the index / dashboard view and hydrates component cards.
- `AboutRoute.cpp`: Secondary information page with client-side SPA navigation (`nova-link`).
- `ApiRoutes.cpp`: Native REST endpoints using `server.json(...)`.

## Route Function Signature

```cpp
#pragma once
#include "../../novacpp/html.hpp"

void renderHomePage(np::RenderContext& np);
void renderAboutPage(np::RenderContext& np);
```
