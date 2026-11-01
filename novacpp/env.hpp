#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <cctype>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <stdlib.h>
#endif

namespace np {

// Sets an environment variable in the current process
inline void setEnvVar(const std::string& key, const std::string& value) {
#ifdef _WIN32
    _putenv_s(key.c_str(), value.c_str());
#else
    setenv(key.c_str(), value.c_str(), 1);
#endif
}

// Retrieves an environment variable with an optional fallback default
inline std::string getEnv(const std::string& key, const std::string& defaultValue = "") {
    const char* val = std::getenv(key.c_str());
    return (val && std::string(val).length() > 0) ? std::string(val) : defaultValue;
}

// Checks if an environment variable exists and is non-empty
inline bool hasEnv(const std::string& key) {
    const char* val = std::getenv(key.c_str());
    return (val && std::string(val).length() > 0);
}

// Loads and parses a standard .env file into the process environment
inline bool loadEnv(const std::string& envPath = ".env", bool overwrite = false) {
    std::ifstream file(envPath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Strip leading whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;

        // Skip comments
        if (line[start] == '#') continue;

        // Strip "export " prefix if present
        std::string trimmed = line.substr(start);
        if (trimmed.rfind("export ", 0) == 0) {
            trimmed = trimmed.substr(7);
            size_t subStart = trimmed.find_first_not_of(" \t");
            if (subStart != std::string::npos) {
                trimmed = trimmed.substr(subStart);
            }
        }

        // Find '=' delimiter
        size_t eqPos = trimmed.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = trimmed.substr(0, eqPos);
        std::string val = trimmed.substr(eqPos + 1);

        // Trim key
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) {
            key.pop_back();
        }

        // Trim val
        size_t vStart = val.find_first_not_of(" \t\r\n");
        if (vStart != std::string::npos) {
            val = val.substr(vStart);
        } else {
            val = "";
        }
        while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\r' || val.back() == '\n')) {
            val.pop_back();
        }

        // Strip surrounding quotes ("..." or '...')
        if (val.length() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
            val = val.substr(1, val.length() - 2);
        }

        if (!key.empty()) {
            if (overwrite || std::getenv(key.c_str()) == nullptr) {
                setEnvVar(key, val);
            }
        }
    }
    return true;
}

} // namespace np
