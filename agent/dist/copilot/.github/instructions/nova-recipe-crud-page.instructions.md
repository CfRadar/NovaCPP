---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP End-to-End Recipe: Interactive CRUD Management Page

<!-- symbols: Database, Component, RenderContext, onForm, onClick -->

## When to use
Use this recipe when creating a complete create, read, update, and delete (CRUD) management dashboard or table view with out-of-band DOM replacement in NovaCPP.

## Prerequisites
- Working knowledge of:
  - [`nova-database`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-database/SKILL.md): SQLite persistence and transactions.
  - [`nova-routing-and-handlers`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-routing-and-handlers/SKILL.md): Route and form registration.
  - [`nova-ui-rendering`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-ui-rendering/SKILL.md): Bento card component layout.
  - [`nova-browser-client`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-browser-client/SKILL.md): `nova-target`, `nova-form`, and `nova-click` DOM wiring.

## Quick start
```cpp
// 1. Create table schema
Database::execute("CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY, name TEXT);");

// 2. Define UI component with surgical DOM target
class ProjectsView : public np::Component {
public:
    ProjectsView() : np::Component("projects-card") {}
    std::string render() override {
        auto rows = Database::query("SELECT id, name FROM projects ORDER BY id DESC;");
        std::string html = "<div class='bento-card col-12' id='projects-card'>";
        html += "<form nova-form='create_project'><input name='name' placeholder='Project name'><button>Add</button></form>";
        html += "<ul>";
        for (const auto& r : rows) {
            html += "<li>" + r.at("name") + " <button nova-click='delete_project' nova-id='" + r.at("id") + "'>Delete</button></li>";
        }
        html += "</ul></div>";
        return html;
    }
};
static ProjectsView s_projectsView;
```

## Steps
1. **Schema Initialization**:
   In `src/backend/Database.cpp`, define table schema during server boot:
   ```cpp
   Database::execute(
       "CREATE TABLE IF NOT EXISTS notes ("
       "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
       "  title TEXT NOT NULL,"
       "  content TEXT NOT NULL"
       ");"
   );
   ```
2. **Component Declaration**:
   Subclass `np::Component` with a stable identifier. Ensure the instance has static lifetime:
   ```cpp
   class NotesManager : public np::Component {
   public:
       NotesManager() : np::Component("notes-manager") {}
       std::string render() override;
   };
   static NotesManager s_notesComp;
   ```
3. **Form Handler (Create & Update)**:
   Register the creation handler on `app`:
   ```cpp
   app.onForm("create_note", [](np::FormEvent e) {
       std::string title = e.get("title");
       std::string content = e.get("content");
       if (!title.empty()) {
           Database::execute("INSERT INTO notes (title, content) VALUES (?, ?);", title, content);
       }
   });
   ```
4. **Click Handler (Delete)**:
   Register deletion by reading the clicked row ID:
   ```cpp
   app.onClick("delete_note", [](np::ClickEvent e) {
       std::string id = e.id;
       Database::execute("DELETE FROM notes WHERE id = ?;", id);
   });
   ```
5. **Route Mounting**:
   Include the component in `src/frontend/App.cpp`:
   ```cpp
   app.route("/notes", [](np::RenderContext& np) {
       np << s_notesComp;
   });
   ```

## Rules
- MUST allocate components in static or shared storage (`static MyComp c` or `std::shared_ptr<MyComp>`).
- MUST use parameterized queries (`Database::execute("... ?", val)`) to prevent SQL injection.
- ALWAYS give the component's root HTML element an `id` that matches the component constructor argument.
- NEVER reload the full page for item deletion; use `nova-click` for seamless out-of-band swaps.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Crash with `std::logic_error` on route | Component allocated on stack inside route lambda | Make component instance `static` or allocate with `std::make_shared` |
| Table does not update after form submission | Form name does not match `onForm` registration | Verify `<form nova-form="name">` equals `app.onForm("name", ...)` |
| Deleted item remains visible | Component root missing `id` matching constructor | Set `<div id="notes-manager">` on the outer card container |

## Verify
Verify table operations and DOM swapping:
```bash
curl -X POST http://localhost:8080/api/nova/form -d "action=create_note&title=Test"
```

## Go deeper
- [`references/crud-architecture.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-recipe-crud-page/references/crud-architecture.md): Entity lifecycle, relational joins, and transactional consistency in NovaCPP.
