# NovaCPP Client Wire Protocol Reference

## Network Endpoints Dispatched by `nova.js`

| HTTP Method | Route Pattern | Request Headers | Request Body | Response Format |
|---|---|---|---|---|
| `POST` | `/nova/action/:name` | `X-Nova-Target: <id>`, `Cookie: nova_session=...` | Empty | `text/html` (Partial snippet or OOB templates) |
| `POST` | `/nova/form/:name` | `Content-Type: application/json`, `X-Nova-Target: <id>` | JSON serialized fields | `text/html` (Updated target component markup) |
| `POST` | `/nova/poll` | `X-Nova-Target: <id>` | Empty | `text/html` (Polled component markup) |
| `POST` | `/nova/navigate` | `X-Nova-Path: /about` | Empty | `text/html` (Inner view markup for SPA transition) |
| `GET` | `/nova/hmr` | `Accept: text/event-stream` | None | SSE Event Stream for Hot Module Replacement |

---

## Out-of-Band (OOB) Swap Mechanism

When multiple component IDs are targeted (`X-Nova-Target: id1, id2`):
1. `ServerContext` isolates each component in memory.
2. The server outputs HTML templates wrapped with `data-nova-swap`:
   ```html
   <template data-nova-swap="id1">
     <div id="id1">...</div>
   </template>
   <template data-nova-swap="id2">
     <div id="id2">...</div>
   </template>
   ```
3. `nova.js` executes:
   ```javascript
   const templates = doc.querySelectorAll('template[data-nova-swap]');
   templates.forEach(tpl => {
       const targetId = tpl.getAttribute('data-nova-swap');
       const targetEl = document.getElementById(targetId);
       if (targetEl) targetEl.outerHTML = tpl.innerHTML;
   });
   ```
