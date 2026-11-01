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
#include <thread>
#include "state.hpp"

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

  // Declarative form helper with action name and optional target element ID
  inline std::string form(const std::string &actionName, 
                          const std::string &targetId = "", 
                          const std::string &className = "") {
    std::string html = "<form nova-form=\"" + actionName + "\"";
    if (!targetId.empty()) {
      html += " nova-target=\"" + targetId + "\"";
    }
    if (!className.empty()) {
      html += " class=\"" + className + "\"";
    }
    html += ">";
    return html;
  }
} // namespace ui
} // namespace np

// Include the cpp-httplib web server (This is massive, so we include it AFTER
// Component definition)
#include "internal/httplib.h"
#include "state.hpp"

namespace np {

// Lightweight JSON parser for form payload maps
inline std::map<std::string, std::string> parseJsonMap(const std::string &jsonStr) {
  std::map<std::string, std::string> result;
  size_t i = 0;
  while (i < jsonStr.length()) {
    size_t keyStart = jsonStr.find('"', i);
    if (keyStart == std::string::npos) break;
    size_t keyEnd = jsonStr.find('"', keyStart + 1);
    if (keyEnd == std::string::npos) break;
    std::string key = jsonStr.substr(keyStart + 1, keyEnd - keyStart - 1);

    size_t colon = jsonStr.find(':', keyEnd + 1);
    if (colon == std::string::npos) break;

    size_t valStart = jsonStr.find_first_not_of(" \t\r\n", colon + 1);
    if (valStart == std::string::npos) break;

    std::string val;
    if (jsonStr[valStart] == '"') {
      size_t valEnd = valStart + 1;
      while (valEnd < jsonStr.length()) {
        if (jsonStr[valEnd] == '"' && jsonStr[valEnd - 1] != '\\') break;
        valEnd++;
      }
      if (valEnd < jsonStr.length()) {
        val = jsonStr.substr(valStart + 1, valEnd - valStart - 1);
        i = valEnd + 1;
      } else {
        break;
      }
    } else {
      size_t valEnd = jsonStr.find_first_of(",}\r\n", valStart);
      if (valEnd == std::string::npos) valEnd = jsonStr.length();
      val = jsonStr.substr(valStart, valEnd - valStart);
      while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back()))) {
        val.pop_back();
      }
      i = valEnd;
    }
    result[key] = val;
  }
  return result;
}

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
  std::map<std::string, std::function<void(const std::map<std::string, std::string>&)>> form_callbacks;
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
  std::atomic<bool> is_shutting_down{false};

  // Background session state cleaner
  std::thread session_cleaner_thread;
  std::condition_variable cleaner_cv;
  std::mutex cleaner_mtx;

public:
  NovaBuilder() = default;

  ~NovaBuilder() {
    if (session_cleaner_thread.joinable()) {
      is_shutting_down = true;
      cleaner_cv.notify_all();
      session_cleaner_thread.join();
    }
  }

  void stop() {
    is_shutting_down = true;
    hmr_active = false;
    hmr_cv.notify_all();
    cleaner_cv.notify_all();
    if (session_cleaner_thread.joinable()) {
      session_cleaner_thread.join();
    }
    svr.stop();
  }

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

  // Register a C++ callback for a form submission with serialized input values
  void onForm(const std::string &actionName,
              std::function<void(const std::map<std::string, std::string> &)> callback) {
    form_callbacks[actionName] = callback;
  }

  // Alias for onForm accepting typed map callback
  void action(const std::string &actionName,
              std::function<void(const std::map<std::string, std::string> &)> callback) {
    form_callbacks[actionName] = callback;
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
      this->stop();
    });

    // Kubernetes Liveness Probe
    svr.Get("/healthz", [](const httplib::Request &/*req*/, httplib::Response &res) {
      res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // Kubernetes Readiness Probe
    svr.Get("/readyz", [this](const httplib::Request &/*req*/, httplib::Response &res) {
      if (this->is_shutting_down.load()) {
        res.status = 503;
        res.set_content("{\"status\":\"draining\"}", "application/json");
        return;
      }
      res.set_content("{\"status\":\"ready\"}", "application/json");
    });

    // Start periodic background session state eviction worker
    session_cleaner_thread = std::thread([this]() {
      while (!this->is_shutting_down.load()) {
        std::unique_lock<std::mutex> lock(this->cleaner_mtx);
        this->cleaner_cv.wait_for(lock, std::chrono::seconds(60), [this]() {
          return this->is_shutting_down.load();
        });
        if (this->is_shutting_down.load()) break;
        np::StateRegistry::instance().evictAll();
      }
    });

    // Endpoint for manual session state eviction
    svr.Post("/nova/session/evict", [](const httplib::Request &/*req*/, httplib::Response &res) {
      size_t count = np::StateRegistry::instance().evictAll();
      res.set_content("{\"status\":\"ok\",\"evicted\":" + std::to_string(count) + "}", "application/json");
    });

    // Endpoint for session logout and state destruction
    svr.Post("/nova/session/logout", [this](const httplib::Request &req, httplib::Response &res) {
      std::string sid = getSessionCookie(req);
      if (!sid.empty()) {
        np::StateRegistry::instance().clearSessionAll(sid);
        {
          std::lock_guard<std::mutex> lock(this->builder_mutex);
          this->session_current_paths.erase(sid);
        }
      }
      res.set_header("Set-Cookie", "nova_session=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
      res.set_content("{\"status\":\"logged_out\"}", "application/json");
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

    // The form route handles serialized form submissions from the browser
    svr.Post(R"(/nova/form/(.*))", [&](const httplib::Request &req,
                                       httplib::Response &res) {
      this->initializeSessionContext(req, res);

      std::string actionName = req.matches[1];

      // Parse payload
      std::map<std::string, std::string> formData;
      if (req.has_header("Content-Type") && 
          req.get_header_value("Content-Type").find("application/json") != std::string::npos) {
        formData = parseJsonMap(req.body);
      } else {
        for (const auto &param : req.params) {
          formData[param.first] = param.second;
        }
      }

      // Execute registered C++ form handler for this specific user session
      if (form_callbacks.find(actionName) != form_callbacks.end()) {
        form_callbacks[actionName](formData);
      } else if (callbacks.find(actionName) != callbacks.end()) {
        callbacks[actionName]();
      }

      // Check if this action targeted a specific component for partial render
      std::string targetId = req.get_header_value("X-Nova-Target");

      if (!targetId.empty() && components.find(targetId) != components.end()) {
        // PARTIAL RENDER: Only re-render the targeted component
        NovaBuilder partialBuilder;
        components[targetId]->render(partialBuilder);
        res.set_content(partialBuilder.generateHTML(false), "text/html");
      } else {
        // FULL RENDER: Re-render the active route for this session
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

      // Ignore static assets and health probes that slipped through
      if (path == "/styles.css" || path == "/nova.js" || path == "/app.js" ||
          path == "/NovaCPP.png" || path == "/healthz" || path == "/readyz" ||
          path.rfind("/assets/", 0) == 0 ||
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
