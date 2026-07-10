#pragma once

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

// Tell the MSVC linker to automatically include winhttp.lib
#pragma comment(lib, "winhttp.lib")

namespace np {

// Fetches data from a REST API over HTTP/HTTPS using Windows Native APIs
inline std::string fetch(const std::string& urlStr, 
                         const std::string& methodStr = "GET", 
                         const std::string& bodyStr = "", 
                         const std::string& headersStr = "") {
    std::string responseData;
    
    // Convert UTF-8 URL to Wide String for Windows API
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &urlStr[0], (int)urlStr.size(), NULL, 0);
    std::wstring wUrl(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &urlStr[0], (int)urlStr.size(), &wUrl[0], size_needed);
    
    // Parse URL components
    URL_COMPONENTS urlComp = { 0 };
    urlComp.dwStructSize = sizeof(urlComp);
    
    wchar_t hostName[256];
    wchar_t urlPath[1024];
    
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(hostName[0]);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(urlPath[0]);
    
    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp)) {
        return "{\"error\":\"Invalid URL\"}";
    }

    // Initialize WinHTTP Session
    HINTERNET hSession = WinHttpOpen(L"NovaCPP/1.0", 
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "{\"error\":\"Session failed\"}";

    // Connect to Host
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "{\"error\":\"Connect failed\"}";
    }

    // Convert method string to wstring
    int method_size = MultiByteToWideChar(CP_UTF8, 0, &methodStr[0], (int)methodStr.size(), NULL, 0);
    std::wstring wMethod(method_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &methodStr[0], (int)methodStr.size(), &wMethod[0], method_size);

    // Prepare Request
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wMethod.c_str(), urlPath,
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            flags);
                                            
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "{\"error\":\"Request failed\"}";
    }

    // Add User-Agent header (Required by some APIs like GitHub)
    LPCWSTR defaultHeader = L"User-Agent: NovaCPP-Framework\r\n";
    WinHttpAddRequestHeaders(hRequest, defaultHeader, -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

    // Add Custom Headers
    if (!headersStr.empty()) {
        int h_size = MultiByteToWideChar(CP_UTF8, 0, &headersStr[0], (int)headersStr.size(), NULL, 0);
        std::wstring wHeaders(h_size, 0);
        MultiByteToWideChar(CP_UTF8, 0, &headersStr[0], (int)headersStr.size(), &wHeaders[0], h_size);
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
            DWORD size = 0;
            DWORD downloaded = 0;
            
            do {
                size = 0;
                WinHttpQueryDataAvailable(hRequest, &size);
                if (size == 0) break;
                
                std::vector<char> buffer(size + 1, 0);
                if (WinHttpReadData(hRequest, (LPVOID)buffer.data(), size, &downloaded)) {
                    responseData.append(buffer.data(), downloaded);
                }
            } while (size > 0);
        }
    } else {
        responseData = "{\"error\":\"Request transmission failed\"}";
    }

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return responseData;
}

} // namespace np
