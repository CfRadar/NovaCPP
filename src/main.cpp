/* ============================================================================
   NOVACPP BACKEND SERVER ENTRYPOINT
   Main orchestration file: environment loader, database pool, and routing.

   ============================================================================
   MINIMAL MAIN BOILERPLATE (To start fresh, you can use this minimal template):
   ============================================================================
   #include "../novacpp/html.hpp"
   #include "../novacpp/env.hpp"
   #include "../novacpp/state.hpp"
   #include "../novacpp/worker_pool.hpp"
   #include "../novacpp/redis_state.hpp"
   #include "backend/Database.hpp"
   #include "backend/Auth.hpp"

   void renderHomePage(np::NovaBuilder& np);

   int main() {
       np::loadEnv(".env");
       Database::connect();
       np::NovaBuilder np;
       np.route("/", renderHomePage);
       np.listen(8080);
       return 0;
   }
   ============================================================================ */

#include "../novacpp/html.hpp"
#include "../novacpp/env.hpp"
#include "../novacpp/state.hpp"
#include "../novacpp/worker_pool.hpp"
#include "../novacpp/redis_state.hpp"
#include "backend/Database.hpp"
#include "backend/Auth.hpp"
#include <csignal>
#include <cstdlib>

// Global app pointer for graceful signal handling in container environments
static np::NovaBuilder* g_runningApp = nullptr;

void signalHandler(int signum) {
    std::cout << "\n[NovaCPP] Caught signal (" << signum << "). Initiating graceful drain..." << std::endl;
    if (g_runningApp) {
        g_runningApp->stop();
    }
}

// Forward declare the page functions from frontend/App.cpp
void renderHomePage(np::NovaBuilder& np);
void renderAboutPage(np::NovaBuilder& np);

int main() {
    // 0. Load .env configuration file if present
    np::loadEnv(".env");

    // Intercept SIGINT and SIGTERM for graceful shutdown
    std::signal(SIGINT, signalHandler);
#if defined(SIGTERM)
    std::signal(SIGTERM, signalHandler);
#endif

    np::NovaBuilder np;
    g_runningApp = &np;

    // Optional: Initialize distributed Redis session store if REDIS_URL is provided
    if (const char* redisUrl = std::getenv("REDIS_URL")) {
        std::cout << "[NovaCPP] Initializing distributed Redis state adapter from REDIS_URL" << std::endl;
        np::useStateStore(np::RedisStateStore::fromUri(redisUrl));
    }
    
    // 1. Initialize SQLite Database Engine (reads DATABASE_URL or defaults to ./data/nova.db with WAL mode)
    Database::connect();
    Database::execute(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  email TEXT NOT NULL UNIQUE,"
        "  role TEXT DEFAULT 'developer'"
        ");"
    );

    // Seed initial demonstration records if table is fresh
    if (Database::query("SELECT id FROM users LIMIT 1").empty()) {
        Database::execute("INSERT INTO users (name, email, role) VALUES ('Ada Lovelace', 'ada@novacpp.dev', 'architect');");
        Database::execute("INSERT INTO users (name, email, role) VALUES ('Alan Turing', 'alan@novacpp.dev', 'core-dev');");
        Database::execute("INSERT INTO users (name, email, role) VALUES ('Margaret Hamilton', 'margaret@novacpp.dev', 'lead');");
    }

    Auth::initializeSessions();
    
    // 2. Register SPA UI Routes
    np.route("/", renderHomePage);
    np.route("/about", renderAboutPage);
    np.route("/users/:id", [](np::NovaBuilder& app, const std::map<std::string, std::string>& params) {
        std::string id = params.count("id") ? params.at("id") : "unknown";
        app << "<div class='bento-grid'>"
            << "  <div class='bento-card col-12'>"
            << "    <h2>User Profile: " + id + "</h2>"
            << "    <p>Dynamic route parameter successfully bound from URL.</p>"
            << "    <a href='/' class='btn-primary' nova-link>&larr; Back to Home</a>"
            << "  </div>"
            << "</div>";
    });
    np.route("/orgs/{org}/repos/{repo}", [](np::NovaBuilder& app, const std::map<std::string, std::string>& params) {
        std::string org = params.count("org") ? params.at("org") : "none";
        std::string repo = params.count("repo") ? params.at("repo") : "none";
        app << "<div class='bento-grid'>"
            << "  <div class='bento-card col-12'>"
            << "    <h2>Repository: " + org + "/" + repo + "</h2>"
            << "    <p>Multi-parameter route successfully bound from URL.</p>"
            << "    <a href='/' class='btn-primary' nova-link>&larr; Back to Home</a>"
            << "  </div>"
            << "</div>";
    });

    // Asynchronous background task trigger demonstration
    np.onClick("async_job", [&np]() {
        np.enqueueTask([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        });
    });

    // 3. Start the Live Server on the Cloud-provided PORT (or default to 8080)
    int port = 8080;
    if (const char* env_p = std::getenv("PORT")) {
        port = std::stoi(env_p);
    }
    
    np.listen(port);
    
    return 0;
}
