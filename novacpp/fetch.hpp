#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <cctype>

namespace np {

// Modern, strongly-typed HTTP response representation
struct FetchResponse {
    int status = 0;
    std::string body;
    std::map<std::string, std::string> headers;

    // Automatic implicit conversion to std::string for 100% backward compatibility
    operator std::string() const noexcept {
        return body;
    }

    bool ok() const noexcept {
        return status >= 200 && status < 300;
    }

    bool isClientError() const noexcept {
        return status >= 400 && status < 500;
    }

    bool isServerError() const noexcept {
        return status >= 500 && status < 600;
    }

    std::string header(const std::string& name, const std::string& fallback = "") const {
        for (const auto& [k, v] : headers) {
            if (k.length() == name.length()) {
                bool match = true;
                for (size_t i = 0; i < k.length(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(k[i])) != std::tolower(static_cast<unsigned char>(name[i]))) {
                        match = false;
                        break;
                    }
                }
                if (match) return v;
            }
        }
        return fallback;
    }
};

} // namespace np

#ifdef _WIN32

#include <windows.h>
#include <winhttp.h>

// Tell the MSVC linker to automatically include winhttp.lib
#pragma comment(lib, "winhttp.lib")

namespace np {

// Thread-safe persistent WinHTTP session managing connection pooling and HTTP keep-alive
class WinHttpSession {
private:
    HINTERNET m_session = nullptr;

public:
    static WinHttpSession& instance() {
        static WinHttpSession s;
        return s;
    }

    WinHttpSession() {
        m_session = WinHttpOpen(
            L"NovaCPP-Framework/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );
        if (m_session) {
            DWORD protocols = WINHTTP_PROTOCOL_FLAG_HTTP2;
            WinHttpSetOption(m_session, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &protocols, sizeof(protocols));
            // Default timeouts: resolve 5s, connect 5s, send 15s, receive 15s
            WinHttpSetTimeouts(m_session, 5000, 5000, 15000, 15000);
        }
    }

    ~WinHttpSession() {
        if (m_session) {
            WinHttpCloseHandle(m_session);
            m_session = nullptr;
        }
    }

    WinHttpSession(const WinHttpSession&) = delete;
    WinHttpSession& operator=(const WinHttpSession&) = delete;

    HINTERNET handle() const {
        return m_session;
    }
};

// Fetches data from a REST API over HTTP/HTTPS using Windows Native APIs with connection pooling
inline FetchResponse fetch(const std::string& urlStr, 
                           const std::string& methodStr = "GET", 
                           const std::string& bodyStr = "", 
                           const std::string& headersStr = "") {
    FetchResponse response;

    // Convert UTF-8 URL to Wide String for Windows API
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, urlStr.c_str(), (int)urlStr.size(), NULL, 0);
    if (size_needed <= 0) {
        response.status = 400;
        response.body = "{\"error\":\"Invalid URL encoding\"}";
        return response;
    }
    std::wstring wUrl(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, urlStr.c_str(), (int)urlStr.size(), &wUrl[0], size_needed);

    // Parse URL components with dynamic buffers to support arbitrary URL & query lengths
    URL_COMPONENTS urlComp = { 0 };
    urlComp.dwStructSize = sizeof(urlComp);

    std::vector<wchar_t> hostBuffer(wUrl.length() + 1, 0);
    std::vector<wchar_t> pathBuffer(wUrl.length() + 1, 0);

    urlComp.lpszHostName = hostBuffer.data();
    urlComp.dwHostNameLength = static_cast<DWORD>(hostBuffer.size());
    urlComp.lpszUrlPath = pathBuffer.data();
    urlComp.dwUrlPathLength = static_cast<DWORD>(pathBuffer.size());

    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp)) {
        response.status = 400;
        response.body = "{\"error\":\"Invalid URL\"}";
        return response;
    }

    // Reuse persistent process-level WinHTTP session handle for socket pooling & keep-alive
    HINTERNET hSession = WinHttpSession::instance().handle();
    bool localSession = false;
    if (!hSession) {
        hSession = WinHttpOpen(L"NovaCPP-Framework/1.0",
                               WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                               WINHTTP_NO_PROXY_NAME,
                               WINHTTP_NO_PROXY_BYPASS, 0);
        localSession = true;
    }
    if (!hSession) {
        response.status = 500;
        response.body = "{\"error\":\"Session failed\"}";
        return response;
    }

    // Connect to Host
    HINTERNET hConnect = WinHttpConnect(hSession, hostBuffer.data(), urlComp.nPort, 0);
    if (!hConnect) {
        if (localSession) WinHttpCloseHandle(hSession);
        response.status = 502;
        response.body = "{\"error\":\"Connect failed\"}";
        return response;
    }

    // Convert method string to wstring
    int method_size = MultiByteToWideChar(CP_UTF8, 0, methodStr.c_str(), (int)methodStr.size(), NULL, 0);
    std::wstring wMethod(method_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, methodStr.c_str(), (int)methodStr.size(), &wMethod[0], method_size);

    // Prepare Request
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wMethod.c_str(), pathBuffer.data(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            flags);
                                            
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        if (localSession) WinHttpCloseHandle(hSession);
        response.status = 500;
        response.body = "{\"error\":\"Request failed\"}";
        return response;
    }

    // Add User-Agent header (Required by some APIs like GitHub)
    LPCWSTR defaultHeader = L"User-Agent: NovaCPP-Framework\r\n";
    WinHttpAddRequestHeaders(hRequest, defaultHeader, -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

    // Add Custom Headers
    if (!headersStr.empty()) {
        int h_size = MultiByteToWideChar(CP_UTF8, 0, headersStr.c_str(), (int)headersStr.size(), NULL, 0);
        std::wstring wHeaders(h_size, 0);
        MultiByteToWideChar(CP_UTF8, 0, headersStr.c_str(), (int)headersStr.size(), &wHeaders[0], h_size);
        WinHttpAddRequestHeaders(hRequest, wHeaders.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    // Prepare Body
    LPVOID pBody = WINHTTP_NO_REQUEST_DATA;
    DWORD bodyLen = 0;
    if (!bodyStr.empty()) {
        pBody = (LPVOID)bodyStr.c_str();
        bodyLen = (DWORD)bodyStr.size();
    }

    // Send Request and Read Response
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, pBody, bodyLen, bodyLen, 0)) {
        if (WinHttpReceiveResponse(hRequest, NULL)) {
            // Extract HTTP status code
            DWORD statusCode = 0;
            DWORD scSize = sizeof(statusCode);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                    WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &scSize, WINHTTP_NO_HEADER_INDEX)) {
                response.status = static_cast<int>(statusCode);
            }

            // Extract HTTP response headers
            DWORD headerSize = 0;
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX,
                                NULL, &headerSize, WINHTTP_NO_HEADER_INDEX);
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && headerSize > 0) {
                std::vector<wchar_t> headerBuf(headerSize / sizeof(wchar_t) + 1, 0);
                if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX,
                                        headerBuf.data(), &headerSize, WINHTTP_NO_HEADER_INDEX)) {
                    std::wstring rawHeaders(headerBuf.data());
                    size_t start = 0;
                    bool isFirstLine = true;
                    while (start < rawHeaders.length()) {
                        size_t end = rawHeaders.find(L"\r\n", start);
                        if (end == std::wstring::npos) {
                            end = rawHeaders.length();
                        }
                        std::wstring line = rawHeaders.substr(start, end - start);
                        start = end + 2;

                        if (isFirstLine) {
                            isFirstLine = false;
                            continue;
                        }
                        if (line.empty()) continue;

                        size_t colon = line.find(L':');
                        if (colon != std::wstring::npos) {
                            std::wstring wKey = line.substr(0, colon);
                            std::wstring wVal = line.substr(colon + 1);

                            size_t first = wVal.find_first_not_of(L" \t");
                            if (first != std::wstring::npos) wVal = wVal.substr(first);
                            size_t last = wVal.find_last_not_of(L" \t\r\n");
                            if (last != std::wstring::npos) wVal = wVal.substr(0, last + 1);

                            int kLen = WideCharToMultiByte(CP_UTF8, 0, wKey.c_str(), (int)wKey.length(), NULL, 0, NULL, NULL);
                            std::string u8Key(kLen, 0);
                            WideCharToMultiByte(CP_UTF8, 0, wKey.c_str(), (int)wKey.length(), &u8Key[0], kLen, NULL, NULL);

                            int vLen = WideCharToMultiByte(CP_UTF8, 0, wVal.c_str(), (int)wVal.length(), NULL, 0, NULL, NULL);
                            std::string u8Val(vLen, 0);
                            WideCharToMultiByte(CP_UTF8, 0, wVal.c_str(), (int)wVal.length(), &u8Val[0], vLen, NULL, NULL);

                            response.headers[u8Key] = u8Val;
                        }
                    }
                }
            }

            // Read response payload
            DWORD size = 0;
            DWORD downloaded = 0;
            do {
                size = 0;
                WinHttpQueryDataAvailable(hRequest, &size);
                if (size == 0) break;
                
                std::vector<char> buffer(size + 1, 0);
                if (WinHttpReadData(hRequest, (LPVOID)buffer.data(), size, &downloaded)) {
                    response.body.append(buffer.data(), downloaded);
                }
            } while (size > 0);
        } else {
            response.status = 502;
            response.body = "{\"error\":\"Failed to receive response\"}";
        }
    } else {
        response.status = 502;
        response.body = "{\"error\":\"Request transmission failed\"}";
    }

    // Cleanup local handles (underlying connection retained in WinHttpSession pool)
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    if (localSession) WinHttpCloseHandle(hSession);

    return response;
}

// Overload accepting structured headers map
inline FetchResponse fetch(const std::string& urlStr,
                           const std::string& methodStr,
                           const std::string& bodyStr,
                           const std::map<std::string, std::string>& headersMap) {
    std::string hStr;
    for (const auto& [k, v] : headersMap) {
        hStr += k + ": " + v + "\r\n";
    }
    return fetch(urlStr, methodStr, bodyStr, hStr);
}

} // namespace np

#else // ==================== LINUX / UNIX FALLBACK ====================

#include "internal/httplib.h"

namespace np {

inline void parseUrl(const std::string& url, std::string& host, std::string& path) {
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        size_t host_start = pos + 3;
        size_t path_start = url.find("/", host_start);
        if (path_start != std::string::npos) {
            host = url.substr(0, path_start);
            path = url.substr(path_start);
        } else {
            host = url;
            path = "/";
        }
    } else {
        host = url;
        path = "/";
    }
}

// Fetches data from a REST API over HTTP/HTTPS using cpp-httplib (Linux fallback)
inline FetchResponse fetch(const std::string& urlStr, 
                           const std::string& methodStr = "GET", 
                           const std::string& bodyStr = "", 
                           const std::string& headersStr = "") {
    FetchResponse response;
    std::string host, path;
    parseUrl(urlStr, host, path);
    
    httplib::Client cli(host);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(10);
    cli.set_keep_alive(true);
    
    httplib::Headers headers = {
        {"User-Agent", "NovaCPP-Framework"}
    };
    
    // Parse custom headers
    size_t start = 0;
    while (start < headersStr.length()) {
        size_t end = headersStr.find("\r\n", start);
        if (end == std::string::npos) break;
        std::string line = headersStr.substr(start, end - start);
        size_t colon = line.find(":");
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            size_t first = val.find_first_not_of(" ");
            if (first != std::string::npos) val = val.substr(first);
            headers.insert({key, val});
        }
        start = end + 2;
    }
    
    httplib::Result res;
    if (methodStr == "GET") {
        res = cli.Get(path, headers);
    } else if (methodStr == "POST") {
        std::string contentType = "text/plain";
        auto it = headers.find("Content-Type");
        if (it != headers.end()) contentType = it->second;
        res = cli.Post(path, headers, bodyStr, contentType);
    } else if (methodStr == "PUT") {
        std::string contentType = "text/plain";
        auto it = headers.find("Content-Type");
        if (it != headers.end()) contentType = it->second;
        res = cli.Put(path, headers, bodyStr, contentType);
    } else if (methodStr == "DELETE") {
        res = cli.Delete(path, headers);
    }
    
    if (res) {
        response.status = res->status;
        response.body = res->body;
        for (const auto& kv : res->headers) {
            response.headers[kv.first] = kv.second;
        }
    } else {
        response.status = 502;
        response.body = "{\"error\":\"Linux Request failed\"}";
    }
    return response;
}

// Overload accepting structured headers map
inline FetchResponse fetch(const std::string& urlStr,
                           const std::string& methodStr,
                           const std::string& bodyStr,
                           const std::map<std::string, std::string>& headersMap) {
    std::string hStr;
    for (const auto& [k, v] : headersMap) {
        hStr += k + ": " + v + "\r\n";
    }
    return fetch(urlStr, methodStr, bodyStr, hStr);
}

} // namespace np

#endif
