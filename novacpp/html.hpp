#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <functional>
#include <map>
#include <random>
#include <ctime>

namespace np {
    class NovaBuilder;
    
    // Component Interface for surgical DOM updates
    class Component {
    public:
        std::string id;
        Component(const std::string& id) : id(id) {}
        virtual void render(NovaBuilder& np) = 0;
        virtual ~Component() = default;
    };
} // namespace np

// Include the cpp-httplib web server (This is massive, so we include it AFTER Component definition)
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
    std::map<std::string, Component*> components; // Map of registered components
    std::map<std::string, std::function<void(NovaBuilder&)>> routes; // Registered pages
    std::string current_path = "/"; // Tracks the user's current SPA route

public:
    void clear() {
        elements.clear();
        // We do NOT clear components here so that polling components remain registered!
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

    // Register a route for the Single Page Application
    void route(const std::string& path, std::function<void(NovaBuilder&)> renderCallback) {
        routes[path] = renderCallback;
    }

    // Register and render a modular component
    void renderComponent(Component& comp, int pollIntervalMs = 0) {
        components[comp.id] = &comp;
        if (pollIntervalMs > 0) {
            elements.push_back("<div id=\"" + comp.id + "\" nova-poll=\"" + std::to_string(pollIntervalMs) + "\">");
        } else {
            elements.push_back("<div id=\"" + comp.id + "\">");
        }
        comp.render(*this);
        elements.push_back("</div>");
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
    void listen(int port) {
        
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

        // SPA Navigation Route (React Router style fast swap)
        svr.Post("/nova/navigate", [&](const httplib::Request& req, httplib::Response& res) {
            this->initializeSessionContext(req, res);
            std::string path = req.get_header_value("X-Nova-Path");
            
            if (routes.find(path) != routes.end()) {
                this->current_path = path;
                this->clear();
                
                // Trigger onLoad for the new route
                if (callbacks.find("__onLoad") != callbacks.end()) {
                    callbacks["__onLoad"]();
                }

                routes[path](*this);
                // Return only inner HTML since it's an SPA transition
                res.set_content(this->generateHTML(false), "text/html");
            } else {
                res.status = 404;
            }
        });

        // The action route handles AJAX requests from the browser
        svr.Post(R"(/nova/action/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
            this->initializeSessionContext(req, res);

            std::string actionName = req.matches[1];
            
            // Execute the registered C++ callback for THIS specific user's state
            if (callbacks.find(actionName) != callbacks.end()) {
                callbacks[actionName]();
            }

            // Check if this action targeted a specific component
            std::string targetId = req.get_header_value("X-Nova-Target");
            
            if (!targetId.empty() && components.find(targetId) != components.end()) {
                // PARTIAL RENDER: Only re-render the targeted component
                NovaBuilder partialBuilder;
                components[targetId]->render(partialBuilder);
                res.set_content(partialBuilder.generateHTML(false), "text/html");
            } else {
                // FULL RENDER: Re-render the current route
                this->clear();
                if (routes.find(this->current_path) != routes.end()) {
                    routes[this->current_path](*this);
                }
                res.set_content(this->generateHTML(false), "text/html");
            }
        });

        // The polling route handles automatic background updates
        svr.Post("/nova/poll", [&](const httplib::Request& req, httplib::Response& res) {
            this->initializeSessionContext(req, res);

            std::string targetId = req.get_header_value("X-Nova-Target");
            if (!targetId.empty() && components.find(targetId) != components.end()) {
                // PARTIAL RENDER: Only re-render the targeted component
                NovaBuilder partialBuilder;
                components[targetId]->render(partialBuilder);
                res.set_content(partialBuilder.generateHTML(false), "text/html");
            } else {
                res.status = 404;
            }
        });

        // Wildcard fallback for direct URL navigation (e.g. hitting refresh on /about)
        // Must be placed at the bottom so it doesn't swallow static files!
        svr.Get(".*", [&](const httplib::Request& req, httplib::Response& res) {
            std::string path = req.path;
            
            // Ignore static assets that slipped through
            if (path == "/styles.css" || path == "/nova.js" || path == "/app.js") return;

            if (routes.find(path) != routes.end()) {
                this->current_path = path;
                this->initializeSessionContext(req, res);

                if (callbacks.find("__onLoad") != callbacks.end()) {
                    callbacks["__onLoad"]();
                }

                this->clear();
                routes[path](*this);
                res.set_content(this->generateHTML(true), "text/html");
            } else {
                res.status = 404;
                res.set_content("<h1>404 Not Found</h1>", "text/html");
            }
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
