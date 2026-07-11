#pragma once
#include <string>

class Auth {
public:
    static void initializeSessions();
    static bool verifyLogin(const std::string& username, const std::string& password);
};
