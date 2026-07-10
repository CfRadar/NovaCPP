#include "../novacpp/html.hpp"

// Forward declare the page functions from App.cpp
void renderHomePage(np::NovaBuilder& np);
void renderAboutPage(np::NovaBuilder& np);

int main() {
    // 1. Create the instance of our framework class
    np::NovaBuilder np;
    
    // 2. Register your SPA Routes
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
