cmake_minimum_required(VERSION 3.10)
project(NovaCPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(NovaCPP 
    src/main.cpp 
    src/frontend/App.cpp 
    src/backend/Database.cpp 
    src/backend/Auth.cpp
    novacpp/sqlite/sqlite3.c
)
target_include_directories(NovaCPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)
target_compile_definitions(NovaCPP PRIVATE SQLITE_ENABLE_WAL=1 SQLITE_ENABLE_JSON1=1)

# Automatically run vendor bundling if Node.js is present
find_program(NODE_EXECUTABLE node)
if(NODE_EXECUTABLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/bundle-vendor.js")
    add_custom_target(bundle-vendor
        COMMAND ${NODE_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/scripts/bundle-vendor.js"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Pre-bundling npm dependencies into render/dist/..."
    )
    add_dependencies(NovaCPP bundle-vendor)
endif()

if(WIN32)
    target_link_libraries(NovaCPP ws2_32)
else()
    # On Linux, find OpenSSL and link it for the cpp-httplib fallback
    find_package(OpenSSL REQUIRED)
    target_link_libraries(NovaCPP OpenSSL::SSL OpenSSL::Crypto pthread ${CMAKE_DL_LIBS})
    target_compile_definitions(NovaCPP PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

# Test simulation target for RAII connection pool on real SQLite DB
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../test/simulate_pool.cpp")
    add_executable(simulate_pool ../test/simulate_pool.cpp novacpp/sqlite/sqlite3.c)
    target_include_directories(simulate_pool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)
    target_compile_definitions(simulate_pool PRIVATE SQLITE_ENABLE_WAL=1 SQLITE_ENABLE_JSON1=1)
    if(WIN32)
        target_link_libraries(simulate_pool ws2_32)
    else()
        target_link_libraries(simulate_pool pthread ${CMAKE_DL_LIBS})
    endif()
endif()

# Custom target to open the HTML file using the default browser
if(WIN32)
    add_custom_target(run-html
        COMMAND cmd.exe /c start "" "http://localhost:8080"
        COMMENT "Opening http://localhost:8080 in the default browser..."
    )
endif()
