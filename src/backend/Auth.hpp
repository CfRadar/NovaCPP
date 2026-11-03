#pragma once

/* ============================================================================
   NOVACPP AUTH SERVICE (src/backend/Auth.hpp)
   Session management, user authentication, and role-based access control.

   USAGE:
   - Call Auth::initializeSessions() during server startup in main.cpp.
   - Use Auth::registerUser(user, pass, role) for new accounts.
   - Use Auth::verifyLogin(user, pass) or Auth::login(user, pass, sid).
   - Use Auth::isAuthenticated(sid) to protect routes.
   - Use Auth::getCurrentUser(sid) and Auth::hasRole(sid, role).
   - Use Auth::logout(sid) on signout.
   ============================================================================ */

#include <string>
#include <unordered_map>
#include <mutex>

struct AuthSession {
    std::string username;
    std::string role;
    long long createdAt = 0;
};

class Auth {
private:
    static std::unordered_map<std::string, AuthSession> s_sessions;
    static std::unordered_map<std::string, std::pair<std::string, std::string>> s_users;
    static std::mutex s_authMutex;

public:
    static void initializeSessions();
    static bool registerUser(const std::string& username, const std::string& password, const std::string& role = "developer");
    static bool verifyLogin(const std::string& username, const std::string& password);
    static bool login(const std::string& username, const std::string& password, const std::string& sessionId);
    static bool isAuthenticated(const std::string& sessionId);
    static std::string getCurrentUser(const std::string& sessionId);
    static bool hasRole(const std::string& sessionId, const std::string& role);
    static void logout(const std::string& sessionId);
};
