#pragma once

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

// Tell the MSVC linker to automatically include winhttp.lib
#pragma comment(lib, "winhttp.lib")

namespace np {

// Fetches data from a REST API over HTTP/HTTPS using Windows Native APIs
inline std::string fetch(const std::string& urlStr) {
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

    // Prepare Request
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            flags);
                                            
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "{\"error\":\"Request failed\"}";
    }

    // Add User-Agent header (Required by some APIs like GitHub)
    LPCWSTR header = L"User-Agent: NovaCPP-Framework\r\n";
    WinHttpAddRequestHeaders(hRequest, header, -1, WINHTTP_ADDREQ_FLAG_ADD);

    // Send Request and Read Response
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
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
