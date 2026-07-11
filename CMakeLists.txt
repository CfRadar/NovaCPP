cmake_minimum_required(VERSION 3.10)
project(NovaCPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(NovaCPP src/main.cpp src/frontend/App.cpp src/backend/Database.cpp src/backend/Auth.cpp)

if(WIN32)
    target_link_libraries(NovaCPP ws2_32)
else()
    # On Linux, find OpenSSL and link it for the cpp-httplib fallback
    find_package(OpenSSL REQUIRED)
    target_link_libraries(NovaCPP OpenSSL::SSL OpenSSL::Crypto pthread)
    target_compile_definitions(NovaCPP PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

# Custom target to open the HTML file using the default browser
if(WIN32)
    add_custom_target(run-html
        COMMAND cmd.exe /c start "" "http://localhost:8080"
        COMMENT "Opening http://localhost:8080 in the default browser..."
    )
endif()
