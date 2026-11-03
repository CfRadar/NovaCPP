# NovaCPP Authentication & Security Policy Reference

## Session Cookie Configuration

NovaCPP configures session cookies with standard defensive attributes:

| Cookie Attribute | Production Setting | Security Justification |
|---|---|---|
| `HttpOnly` | `true` | Prevents Cross-Site Scripting (XSS) attacks from stealing tokens via `document.cookie`. |
| `SameSite` | `Lax` | Protects against Cross-Site Request Forgery (CSRF) on cross-origin requests. |
| `Secure` | Enabled in HTTPS | Prevents transmission over insecure plain-text HTTP connections. |
| `Path` | `/` | Scopes token across all application routes. |

---

## Role-Based Access Control (RBAC) Matrix

```text
Role: anonymous   -> Can access /, /about, /login, /signup
Role: developer   -> Can access /dashboard, /projects, /api/user/*
Role: admin       -> Can access /admin, /users/manage, /system/metrics
```
All role checks query server memory or SQLite database records; client tokens only store the opaque session UUID.

