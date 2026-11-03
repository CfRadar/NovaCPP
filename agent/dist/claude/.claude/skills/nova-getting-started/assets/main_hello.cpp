#include "novacpp/nova.hpp"

void renderHomePage(np::RenderContext& np) {
    np << "<div class='bento-grid'>"
       << "  <div class='bento-card col-12'>"
       << "    <h1>Welcome to NovaCPP</h1>"
       << "    <p>High-performance full-stack C++ web framework.</p>"
       << "  </div>"
       << "</div>";
}

int main() {
    np::loadEnv(".env");
    np::NovaBuilder app;
    app.route("/", renderHomePage);
    app.get("/api/ping", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("pong", "text/plain");
    });
    // For unit testing syntax: do not listen in syntax-only compilation
    return 0;
}

