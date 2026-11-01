#pragma once

/* ============================================================================
   NOVACPP AUTH SERVICE (src/backend/Auth.hpp)
   Session management and user authentication boilerplate.

   USAGE:
   - Call Auth::initializeSessions() during server startup in main.cpp.
   - Use Auth::verifyLogin(user, pass) inside your login route actions.
   ============================================================================ */

#include <string>

class Auth {
public:
    static void initializeSessions();
    static bool verifyLogin(const std::string& username, const std::string& password);
};
