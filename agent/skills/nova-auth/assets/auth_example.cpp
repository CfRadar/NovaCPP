#include "novacpp/nova.hpp"
#include "src/backend/Auth.hpp"
#include <iostream>

void demonstrateAuthentication() {
    Auth::initializeSessions();

    std::string testSession = "test-session-uuid-123";

    // 1. Simulate user registration
    bool regOk = Auth::registerUser("alan_turing", "secret_crypto_pass", "developer");
    std::cout << "User registered: " << (regOk ? "true" : "false") << std::endl;

    // 2. Simulate login attempt
    bool loginOk = Auth::login("alan_turing", "secret_crypto_pass", testSession);
    std::cout << "Login successful: " << (loginOk ? "true" : "false") << std::endl;

    // 3. Inspect authenticated state
    if (Auth::isAuthenticated(testSession)) {
        std::string currentUser = Auth::getCurrentUser(testSession);
        std::cout << "Session authenticated as: " << currentUser << std::endl;
    }

    // 4. Logout session
    Auth::logout(testSession);
    std::cout << "Session after logout: " << (Auth::isAuthenticated(testSession) ? "active" : "destroyed") << std::endl;
}

int main() {
    demonstrateAuthentication();
    return 0;
}

