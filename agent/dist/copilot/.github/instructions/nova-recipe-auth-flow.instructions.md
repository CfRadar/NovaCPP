---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP End-to-End Recipe: Full Authentication & Role Flow

<!-- symbols: Auth, Database, RenderContext, onForm, isAuthenticated -->

## When to use
Use this recipe when building user accounts, credential registration, sign-in forms, cookie-based session tracking, and protected administrator dashboards in NovaCPP.

## Prerequisites
- Working knowledge of:
  - [`nova-auth`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-auth/SKILL.md): `Auth::registerUser`, `Auth::login`, `Auth::isAuthenticated`, `Auth::logout`.
  - [`nova-sessions-state`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-sessions-state/SKILL.md): `np::Context::current_session_id`.
  - [`nova-routing-and-handlers`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-routing-and-handlers/SKILL.md): Form submission and HTTP redirection.
  - [`nova-ui-rendering`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-ui-rendering/SKILL.md): Login card layouts.

## Quick start
```cpp
// 1. Initialize Auth sessions during startup
Auth::initializeSessions();

// 2. Protect route in App.cpp
app.route("/dashboard", [](np::RenderContext& np) {
    std::string sid = np::Context::current_session_id;
    if (!Auth::isAuthenticated(sid)) {
        np.res.set_redirect("/login", 302);
        return;
    }
    std::string user = Auth::getCurrentUser(sid);
    np << "<div class='bento-card col-12'><h1>Welcome " + user + "</h1></div>";
});
```

## Steps
1. **Initialize Auth Subsystem**:
   In `src/main.cpp`, invoke `Auth::initializeSessions();` following database setup:
   ```cpp
   Database::connect();
   Auth::initializeSessions();
   ```
2. **Mount Login & Signup Views**:
   Create the entrypoint forms with Bento Grid styling:
   ```cpp
   app.route("/login", [](np::RenderContext& np) {
       np << "<div class='bento-card col-6'>"
          << "  <h2>Sign In</h2>"
          << "  <form nova-form='do_login'>"
          << "    <input name='user' placeholder='Username' required>"
          << "    <input type='password' name='pass' placeholder='Password' required>"
          << "    <button type='submit'>Login</button>"
          << "  </form>"
          << "</div>";
   });
   ```
3. **Register Login Form Handler**:
   Verify credentials and attach identity to current session:
   ```cpp
   app.onForm("do_login", [](np::FormEvent e) {
       std::string user = e.get("user");
       std::string pass = e.get("pass");
       std::string sid = np::Context::current_session_id;

       if (Auth::login(user, pass, sid)) {
           // Success: session authenticated
       }
   });
   ```
4. **Register Logout Handler**:
   Invalidate the active session token:
   ```cpp
   app.onClick("do_logout", [](np::ClickEvent e) {
       Auth::logout(np::Context::current_session_id);
   });
   ```
5. **Enforce Role-Based Access (RBAC)**:
   Guard sensitive routes using `Auth::hasRole`:
   ```cpp
   if (!Auth::hasRole(sid, "admin")) {
       np << "<h2>403 Forbidden: Admin privileges required</h2>";
       return;
   }
   ```

## Rules
- MUST initialize `Auth::initializeSessions();` before processing incoming HTTP requests.
- MUST guard private routes before generating or rendering protected component content.
- ALWAYS set cookie security attributes (`HttpOnly`, `SameSite=Lax`, `Secure`).
- NEVER pass passwords in cleartext via GET parameters; use POST form events.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| User remains unauthenticated after successful login | Session ID mismatch between cookie and state | Ensure `np::Context::current_session_id` is passed to `Auth::login` |
| Route loop / infinite redirection | Redirecting to protected route when unauthenticated | Ensure `/login` route is public and does not check `isAuthenticated` |
| Admin page accessible to standard users | Checking `isAuthenticated` instead of `hasRole` | Use `Auth::hasRole(sid, "admin")` for privileged views |

## Verify
Verify login and guarded route redirection:
```bash
curl -i http://localhost:8080/dashboard
```
Expect HTTP 302 redirect to `/login` when unauthenticated.

## Go deeper
- [`references/auth-lifecycle.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-recipe-auth-flow/references/auth-lifecycle.md): Complete HTTP cookie lifecycle, cryptographic token signing, and session revocation.
