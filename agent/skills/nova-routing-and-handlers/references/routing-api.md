# NovaCPP Routing & REST API Reference

## `JsonResponse` Helper API

Located in `novacpp/rest_api.hpp` (included via `novacpp/nova.hpp`):

```cpp
namespace np {
    struct JsonResponse {
        int status = 200;
        std::string body;
        std::string content_type = "application/json";

        static JsonResponse ok(std::string json);           // 200 OK
        static JsonResponse created(std::string json);      // 201 Created
        static JsonResponse badRequest(std::string json);   // 400 Bad Request
        static JsonResponse unauthorized(std::string json); // 401 Unauthorized
        static JsonResponse notFound(std::string json);     // 404 Not Found
        static JsonResponse internalError(std::string json);// 500 Internal Error
    };
}
```

---

## Low-Level Raw Server Access

When custom HTTP headers, binary streams, or SSE subscriptions are needed, access the underlying `httplib::Server` directly via:
```cpp
app.rawServer().Get("/custom/stream", [](const httplib::Request& req, httplib::Response& res) {
    res.set_header("Cache-Control", "no-cache");
    res.set_content_provider("text/event-stream", [](size_t offset, httplib::DataSink& sink) {
        sink.write("data: ping\n\n", 12);
        return false;
    });
});
```
