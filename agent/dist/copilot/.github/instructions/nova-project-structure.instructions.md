---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP Project Structure & Monolith Architecture

<!-- symbols: NovaBuilder, RenderContext, Component -->

## When to use
Use this skill when designing application directory layouts, adding new architectural modules, or deciding where to place route definitions, backend logic, and UI components.

## Prerequisites
- A scaffolded NovaCPP project (`nova-getting-started`).

## Quick start
```text
my-app/
├── CMakeLists.txt        # Native C++17 build configuration
├── nova.bat / nova.sh    # Cross-platform development runners
├── .env                  # Runtime configuration (PORT, DATABASE_URL)
├── novacpp/              # Core framework umbrella and runtime headers
├── src/
│   ├── main.cpp          # Backend server orchestration & route registration
│   ├── frontend/
│   │   ├── App.cpp       # Main UI page views, layout trees & event bindings
│   │   └── Boilerplate.cpp # Standalone lightweight template (< 150 lines)
│   ├── backend/
│   │   ├── Database.hpp  # SQLite WAL pool queries and transactions
│   │   └── Auth.hpp      # User authentication, sessions, and credentials
│   ├── components/       # Modular encapsulated UI components (subclassing np::Component)
│   └── routes/           # Decomposed route handlers for larger applications
├── public/               # Static assets (styles.css, images, fonts)
├── render/               # Client runtime (nova.js) and bundled vendor scripts
└── data/                 # Embedded SQLite database storage directory
```

## Steps
1. **Orchestration in `src/main.cpp`**:
   Keep `main.cpp` focused on initialization:
   - Load `.env` configuration.
   - Connect to database pools (`Database::connect()`).
   - Register HTTP SPA routes and native JSON REST endpoints (`np.route`, `np.json`).
   - Listen on the designated `PORT`.
2. **UI Views in `src/frontend/App.cpp`**:
   - Place page rendering functions (`renderHomePage`, `renderAboutPage`).
   - Mount UI components into the 12-column Bento Grid.
   - Register user click actions (`np.onClick`) and form submissions (`np.onForm`).
3. **Modular Components in `src/components/`**:
   - Each component subclasses `np::Component` with a unique ID string.
   - Separate state and markup into reusable cards.
   - Store instances in persistent static storage or pass as `std::shared_ptr`.
4. **Backend Services in `src/backend/`**:
   - Encapsulate business logic, database queries, and third-party API calls.
   - Keep backend logic independent of UI formatting.

## Rules
- MUST keep route registration in `main.cpp` (or decomposed files included into `main.cpp`).
- MUST store UI view definitions in `src/frontend/` or `src/components/`.
- NEVER write raw SQLite queries directly inside UI rendering lambdas; delegate to `src/backend/Database`.
- NEVER create secondary `NovaBuilder` server instances; maintain a single monolithic server.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Circular `#include` dependencies | UI includes backend while backend includes UI | Keep backend agnostic of `App.cpp`; pass data as plain structs or maps |
| Multiple definition linker error (`LNK2005`) | Functions defined in headers without `inline` | Define functions in `.cpp` files or mark header functions as `inline` |
| SQLite database not found | Relative path points outside working directory | Provide absolute path or default to `./data/nova.db` |

## Verify
Check project layout against architectural conventions:
```bash
dir src\frontend, src\backend, src\components
```

## Go deeper
- [`references/monolith-architecture.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-project-structure/references/monolith-architecture.md): Deep dive into shared-memory in-process execution benefits over microservices.
