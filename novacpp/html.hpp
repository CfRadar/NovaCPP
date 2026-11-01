#pragma once
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <random>
#include <string>
#include <vector>

namespace np {
class NovaBuilder;

// Component Interface for surgical DOM updates
class Component {
public:
  std::string id;
  Component(const std::string &id) : id(id) {}
  virtual void render(NovaBuilder &np) = 0;
  virtual ~Component() = default;
};

// Declarative UI helpers for zero-JS npm ecosystem integration
namespace ui {
  // Returns data-nova-effect attribute string for interactive elements
  inline std::string effect(const std::string &effectName) {
    return " data-nova-effect=\"" + effectName + "\"";
  }

  // Returns data-nova-widget mount attribute string for visualizer components
  inline std::string widget(const std::string &widgetName, const std::string &propsJson = "{}") {
    return " data-nova-widget=\"" + widgetName + "\" data-nova-props=\"" + propsJson + "\"";
  }

  // Declarative button with optional built-in npm effect
  inline std::string button(const std::string &label, 
                            const std::string &className = "btn-primary", 
                            const std::string &effectName = "") {
    std::string html = "<button class=\"" + className + "\"";
    if (!effectName.empty()) {
      html += effect(effectName);
    }
    html += ">" + label + "</button>";
    return html;
  }
} // namespace ui
} // namespace np

// Include the cpp-httplib web server (This is massive, so we include it AFTER
// Component definition)
#include "internal/httplib.h"
#include "state.hpp"

namespace np {

// Helper to generate a random session ID
inline std::string generateSessionId() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);
  const char *v = "0123456789abcdef";
  std::string res;
  for (int i = 0; i < 16; i++) {
    res += v[dis(gen)];
  }
  return res;
}

// Helper to extract the nova_session cookie
inline std::string getSessionCookie(const httplib::Request &req) {
  if (req.has_header("Cookie")) {
    std::string cookies = req.get_header_value("Cookie");
    size_t pos = cookies.find("nova_session=");
    if (pos != std::string::npos) {
      size_t end = cookies.find(";", pos);
      if (end == std::string::npos)
        end = cookies.length();
      return cookies.substr(pos + 13, end - (pos + 13));
    }
  }
  return "";
}

struct HMREvent {
  std::string event;
  std::string data;
};

struct HMRClientQueue {
  std::mutex q_mtx;
  std::queue<HMREvent> events;
};

class NovaBuilder {
private:
  std::vector<std::string> elements;
  httplib::Server svr;
  std::map<std::string, std::function<void()>> callbacks;
  std::map<std::string, Component *> components; // Map of registered components
  std::map<std::string, std::function<void(NovaBuilder &)>>
      routes;                     // Registered pages
  std::string current_path = "/"; // Tracks fallback current SPA route
  std::map<std::string, std::string> session_current_paths; // Tracks active path per session
  std::mutex builder_mutex;       // Guards concurrent rendering and route state

  // Real-time HMR Channel members
  std::mutex hmr_clients_mutex;
  std::condition_variable hmr_cv;
  std::vector<std::shared_ptr<HMRClientQueue>> hmr_clients;
  std::atomic<bool> hmr_active{true};

public:
  void broadcastHMR(const std::string &event, const std::string &data = "{}") {
    std::lock_guard<std::mutex> lock(hmr_clients_mutex);
    for (auto &client : hmr_clients) {
      std::lock_guard<std::mutex> q_lock(client->q_mtx);
      client->events.push({event, data});
    }
    hmr_cv.notify_all();
  }
  void clear() {
    elements.clear();
    // We do NOT clear components here so that polling components remain
    // registered!
  }

  void render(const std::string &html_string) {
    elements.push_back(html_string);
  }

  NovaBuilder &operator<<(const std::string &html_string) {
    elements.push_back(html_string);
    return *this;
  }

  // Register a C++ callback for a button action (like React's onClick)
  void onClick(const std::string &actionName, std::function<void()> callback) {
    callbacks[actionName] = callback;
  }

  // Register a C++ callback that runs when the page is loaded/refreshed
  void onLoad(std::function<void()> callback) {
    callbacks["__onLoad"] = callback;
  }

  // Register a route for the Single Page Application
  void route(const std::string &path,
             std::function<void(NovaBuilder &)> renderCallback) {
    routes[path] = renderCallback;
  }

  // Register and render a modular component
  void renderComponent(Component &comp, int pollIntervalMs = 0) {
    components[comp.id] = &comp;
    if (pollIntervalMs > 0) {
      elements.push_back("<div id=\"" + comp.id + "\" nova-poll=\"" +
                         std::to_string(pollIntervalMs) + "\">");
    } else {
      elements.push_back("<div id=\"" + comp.id + "\">");
    }
    comp.render(*this);
    elements.push_back("</div>");
  }

  // Triggers a client-side npm effect upon surgical DOM delivery
  void triggerEffect(const std::string &effectName) {
    elements.push_back("<span data-nova-trigger-effect=\"" + effectName + "\" style=\"display:none;\"></span>");
  }

  // Generate the HTML string
  std::string generateHTML(bool includeWrapper = true) {
    std::string out;

    if (includeWrapper) {
      std::string cache_buster = "?v=" + std::to_string(time(nullptr));
      out += "<!DOCTYPE html>\n";
      out += "<html lang=\"en\">\n<head>\n";
      out += "    <meta charset=\"UTF-8\">\n";
      out += "    <meta name=\"viewport\" content=\"width=device-width, "
             "initial-scale=1.0\">\n";
      out += "    <title>NovaCPP App</title>\n";
      out += "    <link rel=\"stylesheet\" href=\"styles.css" + cache_buster +
             "\">\n";

      // Inject browser import map if available
      std::ifstream im_ifs("./render/dist/importmap.json");
      if (im_ifs) {
        std::string im_content((std::istreambuf_iterator<char>(im_ifs)),
                               std::istreambuf_iterator<char>());
        if (im_content.find("\"imports\": {") != std::string::npos &&
            im_content.find("\"imports\": {}") == std::string::npos) {
          out += "    <script type=\"importmap\">\n" + im_content + "\n    </script>\n";
        }
      }

      out += "</head>\n<body>\n";
      out += "    <div id=\"root\">\n";
    }

    for (const auto &el : elements) {
      out += "        " + el + "\n";
    }

    if (includeWrapper) {
      std::string cache_buster = "?v=" + std::to_string(time(nullptr));
      out += "    </div>\n";
      out += "    <script src=\"nova.js" + cache_buster + "\"></script>\n";
      out += "    <script type=\"module\" src=\"app.js" + cache_buster + "\"></script>\n";
      out += "</body>\n</html>\n";
    }
    return out;
  }

  // Prepare session state for a request
  void initializeSessionContext(const httplib::Request &req,
                                httplib::Response &res) {
    std::string sid = getSessionCookie(req);
    if (sid.empty()) {
      sid = generateSessionId();
      // Set the cookie so the browser remembers its isolated state
      res.set_header("Set-Cookie",
                     "nova_session=" + sid + "; Path=/; HttpOnly");
    }
    // Tell the np::State class which user is currently executing
    np::Context::current_session_id = sid;
  }

  // Starts the Live Server
  void listen(int port) {

    // Mount static asset directories
    svr.set_mount_point("/assets", "./src/assets");

    // Serve static CSS file with strict MIME type
    svr.Get("/styles.css",
            [](const httplib::Request &req, httplib::Response &res) {
              std::ifstream ifs("./render/styles.css");
              if (ifs) {
                std::string content((std::istreambuf_iterator<char>(ifs)),
                                    std::istreambuf_iterator<char>());
                res.set_content(content, "text/css");
              } else {
                res.status = 404;
              }
            });

    // Serve static JS files with strict MIME type
    svr.Get("/nova.js",
            [](const httplib::Request &req, httplib::Response &res) {
              std::ifstream ifs("./render/nova.js");
              if (ifs) {
                std::string content((std::istreambuf_iterator<char>(ifs)),
                                    std::istreambuf_iterator<char>());
                res.set_content(content, "application/javascript");
              } else {
                res.status = 404;
              }
            });

    svr.Get("/app.js", [](const httplib::Request &req, httplib::Response &res) {
      std::ifstream ifs("./render/app.js");
      if (ifs) {
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        res.set_content(content, "application/javascript");
      } else {
        res.status = 404;
      }
    });

    // Serve framework branding logo directly
    svr.Get("/NovaCPP.png", [](const httplib::Request &req, httplib::Response &res) {
      std::ifstream ifs("./src/assets/NovaCPP.png", std::ios::binary);
      if (ifs) {
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        res.set_content(content, "image/png");
      } else {
        res.status = 404;
      }
    });

    // Serve pre-bundled vendor dependencies (/dist/*) with strict security and MIME types
    svr.Get(R"(/dist/(.*))", [](const httplib::Request &req, httplib::Response &res) {
      std::string subpath = req.matches[1];
      if (subpath.find("..") != std::string::npos || subpath.find('\\') != std::string::npos) {
        res.status = 403;
        return;
      }
      std::string full_path = "./render/dist/" + subpath;
      std::ifstream ifs(full_path, std::ios::binary);
      if (ifs) {
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        if (subpath.rfind(".js") == subpath.length() - 3) {
          res.set_content(content, "application/javascript");
        } else if (subpath.rfind(".css") == subpath.length() - 4) {
          res.set_content(content, "text/css");
        } else if (subpath.rfind(".json") == subpath.length() - 5 ||
                   subpath.rfind(".map") == subpath.length() - 4) {
          res.set_content(content, "application/json");
        } else {
          res.set_content(content, "application/octet-stream");
        }
      } else {
        res.status = 404;
      }
    });

    // Server-Sent Events (SSE) Hot Module Replacement Stream
    svr.Get("/nova/hmr", [&](const httplib::Request &req, httplib::Response &res) {
      auto client_queue = std::make_shared<HMRClientQueue>();
      {
        std::lock_guard<std::mutex> lock(this->hmr_clients_mutex);
        client_queue->events.push({"connected", "{\"status\":\"ready\"}"});
        this->hmr_clients.push_back(client_queue);
      }

      res.set_header("Access-Control-Allow-Origin", "*");
      res.set_header("Cache-Control", "no-cache");
      res.set_header("Connection", "keep-alive");

      res.set_chunked_content_provider(
          "text/event-stream",
          [this, client_queue](size_t /*offset*/, httplib::DataSink &sink) {
            if (!sink.is_writable() || !this->hmr_active) {
              return false;
            }

            HMREvent next_ev;
            bool has_event = false;

            {
              std::unique_lock<std::mutex> lock(this->hmr_clients_mutex);
              {
                std::lock_guard<std::mutex> q_lock(client_queue->q_mtx);
                if (!client_queue->events.empty()) {
                  next_ev = client_queue->events.front();
                  client_queue->events.pop();
                  has_event = true;
                }
              }

              if (!has_event) {
                this->hmr_cv.wait_for(lock, std::chrono::seconds(2), [&]() {
                  if (!this->hmr_active) return true;
                  std::lock_guard<std::mutex> q_lock(client_queue->q_mtx);
                  return !client_queue->events.empty();
                });

                if (!this->hmr_active || !sink.is_writable()) {
                  return false;
                }

                std::lock_guard<std::mutex> q_lock(client_queue->q_mtx);
                if (!client_queue->events.empty()) {
                  next_ev = client_queue->events.front();
                  client_queue->events.pop();
                  has_event = true;
                }
              }
            }

            if (has_event) {
              std::string payload = "event: " + next_ev.event + "\ndata: " + next_ev.data + "\n\n";
              return sink.write(payload.data(), payload.size());
            } else {
              std::string ping = ": ping\n\n";
              return sink.write(ping.data(), ping.size());
            }
          },
          [this, client_queue](bool /*success*/) {
            std::lock_guard<std::mutex> lock(this->hmr_clients_mutex);
            this->hmr_clients.erase(
                std::remove(this->hmr_clients.begin(), this->hmr_clients.end(), client_queue),
                this->hmr_clients.end());
          });
    });

    // Endpoint for dev runners to broadcast hot reload events to connected browsers
    svr.Post("/nova/hmr/broadcast", [&](const httplib::Request &req, httplib::Response &res) {
      std::string event = req.get_header_value("X-Nova-HMR-Event");
      if (event.empty()) event = "component-update";
      std::string data = req.body.empty() ? "{}" : req.body;
      this->broadcastHMR(event, data);
      res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // Graceful stop endpoint for dev reloader
    svr.Post("/nova/stop", [&](const httplib::Request &req, httplib::Response &res) {
      res.set_content("{\"status\":\"stopping\"}", "application/json");
      this->hmr_active = false;
      this->hmr_cv.notify_all();
      this->svr.stop();
    });

    // SPA Navigation Route (React Router style fast swap)
    svr.Post("/nova/navigate",
             [&](const httplib::Request &req, httplib::Response &res) {
               this->initializeSessionContext(req, res);
               std::string path = req.get_header_value("X-Nova-Path");
               std::string sid = np::Context::current_session_id;

               if (routes.find(path) != routes.end()) {
                 std::lock_guard<std::mutex> lock(this->builder_mutex);
                 this->session_current_paths[sid] = path;
                 this->current_path = path;
                 this->clear();

                 bool isHMR = req.has_header("X-Nova-HMR");
                 // Trigger onLoad for route transitions, but preserve state during HMR Fast Refresh
                 if (!isHMR && callbacks.find("__onLoad") != callbacks.end()) {
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
    svr.Post(R"(/nova/action/(.*))", [&](const httplib::Request &req,
                                         httplib::Response &res) {
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
        // FULL RENDER: Re-render the current route for this specific session
        std::lock_guard<std::mutex> lock(this->builder_mutex);
        std::string sid = np::Context::current_session_id;
        std::string active_path = this->session_current_paths.count(sid)
                                      ? this->session_current_paths[sid]
                                      : this->current_path;
        this->clear();
        if (routes.find(active_path) != routes.end()) {
          routes[active_path](*this);
        }
        res.set_content(this->generateHTML(false), "text/html");
      }
    });

    // The polling route handles automatic background updates
    svr.Post("/nova/poll", [&](const httplib::Request &req,
                               httplib::Response &res) {
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

    // Wildcard fallback for direct URL navigation (e.g. hitting refresh on
    // /about) Must be placed at the bottom so it doesn't swallow static files!
    svr.Get(".*", [&](const httplib::Request &req, httplib::Response &res) {
      std::string path = req.path;

      // Ignore static assets that slipped through
      if (path == "/styles.css" || path == "/nova.js" || path == "/app.js" ||
          path == "/NovaCPP.png" || path.rfind("/assets/", 0) == 0 ||
          path.rfind("/dist/", 0) == 0)
        return;

      if (routes.find(path) != routes.end()) {
        std::lock_guard<std::mutex> lock(this->builder_mutex);
        this->initializeSessionContext(req, res);
        std::string sid = np::Context::current_session_id;
        this->session_current_paths[sid] = path;
        this->current_path = path;

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

    std::cout << "NovaCPP: Live Server running! Open http://localhost:" << port
              << " in your browser.\n";

    if (!svr.listen("0.0.0.0", port)) {
      std::cerr << "\n[NovaCPP ERROR] Failed to start server! Port " << port
                << " is already in use.\n";
      std::cerr << "Are you already running another NovaCPP app (like the "
                   "stopwatch) in the background?\n";
      std::cerr << "Please close the other terminal or kill the process and "
                   "try again.\n\n";
    }
  }
};

} // namespace np
