#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <functional>
#include <map>
#include <random>
#include <ctime>

// Include the cpp-httplib web server
#include "internal/httplib.h"
#include "state.hpp"

namespace np {

// Helper to generate a random session ID
inline std::string generateSessionId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    const char* v = "0123456789abcdef";
    std::string res;
    for (int i = 0; i < 16; i++) {
        res += v[dis(gen)];
    }
    return res;
}

// Helper to extract the nova_session cookie
inline std::string getSessionCookie(const httplib::Request& req) {
    if (req.has_header("Cookie")) {
        std::string cookies = req.get_header_value("Cookie");
        size_t pos = cookies.find("nova_session=");
        if (pos != std::string::npos) {
            size_t end = cookies.find(";", pos);
            if (end == std::string::npos) end = cookies.length();
            return cookies.substr(pos + 13, end - (pos + 13));
        }
    }
    return "";
}

class NovaBuilder {
private:
    std::vector<std::string> elements;
    httplib::Server svr;
    std::map<std::string, std::function<void()>> callbacks;

public:
    void clear() {
        elements.clear();
    }

    void render(const std::string& html_string) {
        elements.push_back(html_string);
    }

    NovaBuilder& operator<<(const std::string& html_string) {
        elements.push_back(html_string);
        return *this;
    }

    // Register a C++ callback for a button action (like React's onClick)
    void onClick(const std::string& actionName, std::function<void()> callback) {
        callbacks[actionName] = callback;
    }

    // Register a C++ callback that runs when the page is loaded/refreshed
    void onLoad(std::function<void()> callback) {
        callbacks["__onLoad"] = callback;
    }

    // Generate the HTML string
    std::string generateHTML(bool includeWrapper = true) {
        std::string out;
        
        if (includeWrapper) {
            std::string cache_buster = "?v=" + std::to_string(time(nullptr));
            out += "<!DOCTYPE html>\n";
            out += "<html lang=\"en\">\n<head>\n";
            out += "    <meta charset=\"UTF-8\">\n";
            out += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
            out += "    <title>NovaCPP App</title>\n";
            out += "    <link rel=\"stylesheet\" href=\"styles.css" + cache_buster + "\">\n";
            out += "</head>\n<body>\n";
            out += "    <div id=\"root\">\n";
        }

        for (const auto& el : elements) {
            out += "        " + el + "\n";
        }

        if (includeWrapper) {
            std::string cache_buster = "?v=" + std::to_string(time(nullptr));
            out += "    </div>\n";
            out += "    <script src=\"nova.js" + cache_buster + "\"></script>\n";
            out += "    <script src=\"app.js" + cache_buster + "\"></script>\n";
            out += "</body>\n</html>\n";
        }
        return out;
    }

    // Prepare session state for a request
    void initializeSessionContext(const httplib::Request& req, httplib::Response& res) {
        std::string sid = getSessionCookie(req);
        if (sid.empty()) {
            sid = generateSessionId();
            // Set the cookie so the browser remembers its isolated state
            res.set_header("Set-Cookie", "nova_session=" + sid + "; Path=/; HttpOnly");
        }
        // Tell the np::State class which user is currently executing
        np::Context::current_session_id = sid;
    }

    // Starts the Live Server
    void listen(int port, std::function<void(NovaBuilder&)> renderCallback) {
        
        // Serve static CSS file with strict MIME type
        svr.Get("/styles.css", [](const httplib::Request& req, httplib::Response& res) {
            std::ifstream ifs("./render/styles.css");
            if (ifs) {
                std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                res.set_content(content, "text/css");
            } else { res.status = 404; }
        });

        // Serve static JS files with strict MIME type
        svr.Get("/nova.js", [](const httplib::Request& req, httplib::Response& res) {
            std::ifstream ifs("./render/nova.js");
            if (ifs) {
                std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                res.set_content(content, "application/javascript");
            } else { res.status = 404; }
        });
        
        svr.Get("/app.js", [](const httplib::Request& req, httplib::Response& res) {
            std::ifstream ifs("./render/app.js");
            if (ifs) {
                std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                res.set_content(content, "application/javascript");
            } else { res.status = 404; }
        });

        // The root route renders the entire HTML shell
        svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
            this->initializeSessionContext(req, res);

            // Trigger onLoad lifecycle hook if registered
            if (callbacks.find("__onLoad") != callbacks.end()) {
                callbacks["__onLoad"]();
            }

            this->clear();
            renderCallback(*this);
            res.set_content(this->generateHTML(true), "text/html");
        });

        // The action route handles AJAX requests from the browser
        svr.Post(R"(/nova/action/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
            this->initializeSessionContext(req, res);

            std::string actionName = req.matches[1];
            
            // Execute the registered C++ callback for THIS specific user's state
            if (callbacks.find(actionName) != callbacks.end()) {
                callbacks[actionName]();
            }

            // Re-render the app based on the user's isolated state
            this->clear();
            renderCallback(*this);
            
            // Send ONLY the inner HTML back to the browser
            res.set_content(this->generateHTML(false), "text/html");
        });

        std::cout << "NovaCPP: Live Server running! Open http://localhost:" << port << " in your browser.\n";
        
        if (!svr.listen("0.0.0.0", port)) {
            std::cerr << "\n[NovaCPP ERROR] Failed to start server! Port " << port << " is already in use.\n";
            std::cerr << "Are you already running another NovaCPP app (like the stopwatch) in the background?\n";
            std::cerr << "Please close the other terminal or kill the process and try again.\n\n";
        }
    }
};

} // namespace np
