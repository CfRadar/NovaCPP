#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

#include <cstdlib>
#include <string>
#include <optional>

namespace np {

// Safe cross-platform environment variable accessor returning std::optional
inline std::optional<std::string> getEnvOptional(const std::string& varName) {
#if defined(_MSC_VER)
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, varName.c_str()) == 0 && buf != nullptr) {
        std::string result(buf);
        free(buf);
        return result;
    }
    return std::nullopt;
#else
    const char* val = std::getenv(varName.c_str());
    if (val != nullptr) {
        return std::string(val);
    }
    return std::nullopt;
#endif
}

// Safe cross-platform environment variable accessor with default value fallback
inline std::string getEnvOr(const std::string& varName, const std::string& defaultValue) {
    auto opt = getEnvOptional(varName);
    return (opt.has_value() && !opt.value().empty()) ? opt.value() : defaultValue;
}

// Direct accessor returning std::string with optional default value
inline std::string getEnv(const std::string& varName, const std::string& defaultValue = "") {
    return getEnvOr(varName, defaultValue);
}

// Checks if an environment variable exists and is non-empty
inline bool hasEnv(const std::string& varName) {
    auto opt = getEnvOptional(varName);
    return opt.has_value() && !opt.value().empty();
}

} // namespace np
