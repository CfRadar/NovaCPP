# Authentication Lifecycle & Cookie Security

## Authentication State Transitions
```
[Anonymous Visitor]
       |
       v (POST /api/nova/form with "do_login")
[Credential Verification via Auth::login]
       |
       +---> [Failure] -> Re-render form with error message
       |
       v [Success]
[Session Token Generated & Bound to sid]
       |
       v (Set-Cookie: nova_session=sid; HttpOnly; SameSite=Lax; Path=/)
[Authenticated Navigation to /dashboard]
       |
       v (POST /api/nova/click with "do_logout")
[Session Invalidation via Auth::logout]
```

## Security Best Practices
- **HttpOnly**: Prevents JavaScript `document.cookie` access, protecting against XSS token exfiltration.
- **SameSite=Lax**: Prevents Cross-Site Request Forgery (CSRF) on external navigation.
- **Secure**: Forces transmission exclusively over HTTPS in production environments.

