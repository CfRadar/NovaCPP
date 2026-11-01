#include "../novacpp/html.hpp"
#include "backend/Database.hpp"
#include "backend/Auth.hpp"
#include <csignal>

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
    // Intercept SIGINT and SIGTERM for graceful shutdown
    std::signal(SIGINT, signalHandler);
#if defined(SIGTERM)
    std::signal(SIGTERM, signalHandler);
#endif

    np::NovaBuilder np;
    g_runningApp = &np;
    
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
    Auth::initializeSessions();
    
    // 2. Register SPA UI Routes
    np.route("/", renderHomePage);
    np.route("/about", renderAboutPage);

    // 3. Start the Live Server on the Cloud-provided PORT (or default to 8080)
    int port = 8080;
    if (const char* env_p = std::getenv("PORT")) {
        port = std::stoi(env_p);
    }
    
    np.listen(port);
    
    return 0;
}
