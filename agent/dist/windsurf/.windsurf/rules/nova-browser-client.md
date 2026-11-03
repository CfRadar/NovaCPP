# NovaCPP Rule: nova-browser-client

# NovaCPP Browser Client & Wire Protocol

<!-- symbols: NovaBuilder, RenderContext, Component -->

## When to use
Use this skill when configuring frontend event listeners, binding buttons without full-page reloads, sending surgical updates to specific DOM elements, submitting forms via AJAX, or configuring background polling.

## Prerequisites
- A NovaCPP application serving `render/nova.js`.
- Components rendered with unique IDs.

## Quick start
```html
<!-- 1. Single-Target Surgical Click -->
<button class="btn-primary" nova-click="increment" nova-target="counter-card">
  + Increment
</button>

<!-- 2. Dual-Target Out-of-Band Swap -->
<button class="btn-secondary" nova-click="refresh_all" nova-target="counter-card, clock-card">
  Refresh All
</button>

<!-- 3. Two-Way Form Binding -->
<form nova-form="save_note" nova-target="notes-card">
  <input type="text" name="note_text" class="input-control" required />
  <button type="submit" class="btn-primary">Save Note</button>
</form>

<!-- 4. Background Visibility Polling -->
<div id="status-card" nova-poll="5000">
  <p>Live status updates every 5s...</p>
</div>
```

## HTML Directives Reference

| Attribute | Value Syntax | Behavioral Description |
|---|---|---|
| `nova-click` | `action_name` | Intercepts click; dispatches `POST /nova/action/<action_name>`. |
| `nova-target` | `comp-id` or `id1, id2` | Sets `X-Nova-Target` header; server returns only the targeted component HTML without full document re-render. |
| `nova-form` | `handler_name` | Intercepts `<form>` submit; serializes input fields into JSON payload; dispatches to `POST /nova/form/<handler_name>`. |
| `nova-poll` | Milliseconds (e.g. `3000`) | Issues background `POST /nova/poll` with `X-Nova-Target`. Automatically throttles to >=15s when browser tab is hidden (`document.hidden`). |
| `nova-link` | Boolean attribute | Intercepts `<a>` clicks for client-side SPA navigation without full browser reload. Dispatches `POST /nova/navigate`. |

## Multi-Target Surgical Updates
When `nova-target="comp-a, comp-b"` is specified:
1. The server generates out-of-band template wrappers:
   ```html
   <template data-nova-swap="comp-a"><div>Updated A</div></template>
   <template data-nova-swap="comp-b"><div>Updated B</div></template>
   ```
2. The server sets header `X-Nova-Multi-Target: true`.
3. `nova.js` extracts each template and surgical updates `document.getElementById('comp-a')` and `comp-b` respectively without touching the rest of the DOM tree.

## Rules
- MUST ensure `nova-target` values match the ID passed to the C++ `Component("id")` constructor.
- MUST wrap form elements inside a `<form nova-form="...">` for automatic serialization.
- NEVER include `#` inside `nova-target` (use `nova-target="counter-card"`, not `#counter-card`).
- NEVER manually attach native `onclick` handlers that fight with `nova-click`.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Entire page flickers on click | Button missing `nova-target` attribute | Add `nova-target="comp-id"` to limit re-rendering to that card |
| Target not updated | Target string contains `#` | Remove `#` prefix: `nova-target="comp-id"` |
| Polling continues at high rate in background | Missing page visibility handling | Ensure `render/nova.js` is loaded; it automatically throttles background polling |

## Verify
Open developer tools Network tab, click a `nova-click` button, and inspect the response:
- URL: `POST /nova/action/<name>`
- Request Header: `X-Nova-Target: <id>`
- Response Body: Partial HTML snippet starting with `<div id="<id>">` without `<!DOCTYPE html>`.

## Go deeper
- [`references/wire-protocol.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-browser-client/references/wire-protocol.md): Detailed byte-level wire protocol specification for `nova.js`.
