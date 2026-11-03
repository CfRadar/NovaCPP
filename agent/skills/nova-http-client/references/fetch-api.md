# NovaCPP Outbound HTTP Client Reference

## `FetchResponse` Struct Definition

Located in `novacpp/fetch.hpp`:

```cpp
namespace np {
    struct FetchResponse {
        int status = 0;
        std::string body;
        std::map<std::string, std::string> headers;

        bool isOk() const { return status >= 200 && status < 300; }
        std::string json() const { return body; }
        std::string jsonField(const std::string& key) const;
    };

    // Convenience Functions with std::map<string, string> headers
    FetchResponse fetch(const std::string& url,
                        const std::map<std::string, std::string>& headers = {},
                        const std::string& method = "GET",
                        const std::string& body = "",
                        int timeoutMs = 10000);

    FetchResponse get(const std::string& url, const std::map<std::string, std::string>& headers = {}, int timeoutMs = 10000);
    FetchResponse post(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers = {}, int timeoutMs = 10000);
    FetchResponse put(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers = {}, int timeoutMs = 10000);
    FetchResponse del(const std::string& url, const std::map<std::string, std::string>& headers = {}, int timeoutMs = 10000);
}
```
