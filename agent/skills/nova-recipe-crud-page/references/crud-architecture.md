# CRUD Architecture & Data Flow in NovaCPP

## Component-Level Swapping Flow
1. **Initial Mount**: Server streams full page HTML on direct browser GET request.
2. **Form Submission**: Client `nova.js` catches form submit, transmits payload via POST `/api/nova/form` with `nova-target="projects-card"`.
3. **Execution**: Server invokes `app.onForm` handler, mutates SQLite database, and re-renders only `ProjectsView`.
4. **Targeted DOM Patch**: `nova.js` replaces `#projects-card` in the live DOM without full page refresh or loss of scroll position.

