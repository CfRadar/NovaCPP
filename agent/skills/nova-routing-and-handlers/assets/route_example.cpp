#include "novacpp/nova.hpp"

void renderHomePage(np::RenderContext& np) {
    np << "<div class='bento-grid'><div class='bento-card col-12'><h1>Routing Demo</h1></div></div>";
}

int main() {
    np::loadEnv(".env");
    np::NovaBuilder app;

    // 1. Static HTML Route
    app.route("/", renderHomePage);

    // 2. Parameterized Route
    app.route("/users/:id", [](np::RenderContext& np, const std::map<std::string, std::string>& params) {
        std::string id = params.count("id") ? params.at("id") : "0";
        np << "<h2>User: " + id + "</h2>";
    });

    // 3. Native JSON REST API with status factory
    app.json("/api/status", [](const httplib::Request&) -> np::JsonResponse {
        return np::JsonResponse::ok(R"({"status":"online","framework":"NovaCPP"})");
    });

    // 4. Method-restricted REST API
    app.json("/api/login", "POST", [](const httplib::Request& req) -> np::JsonResponse {
        if (req.body.find("secret") != std::string::npos) {
            return np::JsonResponse::ok(R"({"token":"auth-token-123"})");
        }
        return np::JsonResponse::unauthorized(R"({"error":"Invalid credentials"})");
    });

    return 0;
}
