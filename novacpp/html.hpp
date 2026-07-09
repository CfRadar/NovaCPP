#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <functional>
#include <map>

// Include the cpp-httplib web server
#include "internal/httplib.h"

namespace np {

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

    // Generate the HTML string
    std::string generateHTML(bool includeWrapper = true) {
        std::string out;
        
        if (includeWrapper) {
            out += "<!DOCTYPE html>\n";
            out += "<html lang=\"en\">\n<head>\n";
            out += "    <meta charset=\"UTF-8\">\n";
            out += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
            out += "    <title>NovaCPP App</title>\n";
            out += "    <link rel=\"stylesheet\" href=\"styles.css\">\n";
            out += "</head>\n<body>\n";
            out += "    <div id=\"root\">\n";
        }

        for (const auto& el : elements) {
            out += "        " + el + "\n";
        }

        if (includeWrapper) {
            out += "    </div>\n";
            out += "    <script src=\"nova.js\"></script>\n";
            out += "    <script src=\"app.js\"></script>\n";
            out += "</body>\n</html>\n";
        }
        return out;
    }

    // Starts the Live Server instead of just writing a static file
    void listen(int port, std::function<void(NovaBuilder&)> renderCallback) {
        // Serve static files from the render directory (like styles.css)
        svr.set_mount_point("/", "./render");

        // The root route renders the entire HTML shell
        svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
            this->clear();
            renderCallback(*this);
            res.set_content(this->generateHTML(true), "text/html");
        });

        // The action route handles AJAX requests from the browser
        svr.Post(R"(/nova/action/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
            std::string actionName = req.matches[1];
            
            // Execute the registered C++ callback
            if (callbacks.find(actionName) != callbacks.end()) {
                callbacks[actionName]();
            }

            // Re-render the app based on the new state
            this->clear();
            renderCallback(*this);
            
            // Send ONLY the inner HTML back to the browser to inject into the DOM
            res.set_content(this->generateHTML(false), "text/html");
        });

        std::cout << "NovaCPP: Live Server running! Open http://localhost:" << port << " in your browser.\n";
        svr.listen("0.0.0.0", port);
    }
};

} // namespace np
