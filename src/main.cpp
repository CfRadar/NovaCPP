#include "../novacpp/html.hpp"
#include "backend/Database.hpp"
#include "backend/Auth.hpp"

// Forward declare the page functions from frontend/App.cpp
void renderHomePage(np::NovaBuilder& np);
void renderAboutPage(np::NovaBuilder& np);

int main() {
    np::NovaBuilder np;
    
    // 1. Initialize Pure Backend Services
    Database::connect("postgres://nova_admin:secret@localhost:5432/nova_db");
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
