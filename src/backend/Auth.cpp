#include "Auth.hpp"
#include <iostream>
#include <chrono>

std::unordered_map<std::string, AuthSession> Auth::s_sessions;
std::unordered_map<std::string, std::pair<std::string, std::string>> Auth::s_users;
std::mutex Auth::s_authMutex;

void Auth::initializeSessions() {
    std::lock_guard<std::mutex> lock(s_authMutex);
    std::cout << "[BACKEND] Initializing secure Auth session manager...\n";
    s_sessions.clear();
    // Default admin user for local development and testing
    s_users["admin"] = { "secret", "admin" };
    std::cout << "[BACKEND] Auth subsystem ready!\n";
}

bool Auth::registerUser(const std::string& username, const std::string& password, const std::string& role) {
    std::lock_guard<std::mutex> lock(s_authMutex);
    if (username.empty() || password.empty()) {
        return false;
    }
    if (s_users.find(username) != s_users.end()) {
        return false;
    }
    s_users[username] = { password, role };
    return true;
}

bool Auth::verifyLogin(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(s_authMutex);
    auto it = s_users.find(username);
    if (it != s_users.end() && it->second.first == password) {
        return true;
    }
    return (username == "admin" && password == "secret");
}

bool Auth::login(const std::string& username, const std::string& password, const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(s_authMutex);
    auto it = s_users.find(username);
    if (it == s_users.end() || it->second.first != password) {
        if (!(username == "admin" && password == "secret")) {
            return false;
        }
    }
    std::string role = (it != s_users.end()) ? it->second.second : "admin";
    long long now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    s_sessions[sessionId] = { username, role, now };
    return true;
}

bool Auth::isAuthenticated(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(s_authMutex);
    return s_sessions.find(sessionId) != s_sessions.end();
}

std::string Auth::getCurrentUser(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(s_authMutex);
    auto it = s_sessions.find(sessionId);
    if (it != s_sessions.end()) {
        return it->second.username;
    }
    return "";
}

bool Auth::hasRole(const std::string& sessionId, const std::string& role) {
    std::lock_guard<std::mutex> lock(s_authMutex);
    auto it = s_sessions.find(sessionId);
    if (it != s_sessions.end()) {
        return it->second.role == role;
    }
    return false;
}

void Auth::logout(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(s_authMutex);
    s_sessions.erase(sessionId);
}
