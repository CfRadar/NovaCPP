# Server-Sent Events (SSE) Protocol Specification

## HTTP Wire Headers
When establishing an SSE stream endpoint:
```http
HTTP/1.1 200 OK
Content-Type: text/event-stream; charset=utf-8
Cache-Control: no-cache, no-transform
Connection: keep-alive
X-Accel-Buffering: no
```

## Event Taxonomy
1. `event: stream-token`: Token delta emitted during generation.
2. `event: node-start`: Signal indicating a graph node started execution.
3. `event: node-end`: Signal indicating a graph node finished execution.
4. `event: tool-start`: Signal indicating an external tool call was dispatched.
5. `event: tool-end`: Signal indicating tool completion with result status.
6. `event: state-update`: Complete serialized snapshot of active state.

## Browser Client Reconnection
`nova.js` maintains an `EventSource` connection and reconnects automatically upon transport interruption.

