cmake_minimum_required(VERSION 3.10)
project(NovaCPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile SQLite amalgamation once into an isolated reusable static library
add_library(sqlite3_static STATIC novacpp/sqlite/sqlite3.c)
target_include_directories(sqlite3_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/novacpp/sqlite)
target_compile_definitions(sqlite3_static PUBLIC SQLITE_ENABLE_WAL=1 SQLITE_ENABLE_JSON1=1)

add_executable(NovaCPP 
    src/main.cpp 
    src/frontend/App.cpp 
    src/backend/Database.cpp 
    src/backend/Auth.cpp
)
target_include_directories(NovaCPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)

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
    target_link_libraries(NovaCPP PRIVATE sqlite3_static ws2_32)
else()
    # On Linux, find OpenSSL and link it for the cpp-httplib fallback
    find_package(OpenSSL REQUIRED)
    target_link_libraries(NovaCPP PRIVATE sqlite3_static OpenSSL::SSL OpenSSL::Crypto pthread ${CMAKE_DL_LIBS})
    target_compile_definitions(NovaCPP PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

# Test simulation target for RAII connection pool on real SQLite DB
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Test/simulate_pool.cpp")
    add_executable(simulate_pool ../Test/simulate_pool.cpp)
    target_include_directories(simulate_pool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)
    if(WIN32)
        target_link_libraries(simulate_pool PRIVATE sqlite3_static ws2_32)
    else()
        target_link_libraries(simulate_pool PRIVATE sqlite3_static pthread ${CMAKE_DL_LIBS})
    endif()
endif()

# Test simulation target for Session State TTL and Eviction
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Test/simulate_state_ttl.cpp")
    add_executable(simulate_state_ttl ../Test/simulate_state_ttl.cpp)
    target_include_directories(simulate_state_ttl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)
    if(WIN32)
        target_link_libraries(simulate_state_ttl ws2_32)
    else()
        target_link_libraries(simulate_state_ttl pthread)
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Test/simulate_worker_pool.cpp")
    add_executable(simulate_worker_pool ../Test/simulate_worker_pool.cpp)
    target_include_directories(simulate_worker_pool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)
    if(WIN32)
        target_link_libraries(simulate_worker_pool ws2_32)
    else()
        target_link_libraries(simulate_worker_pool pthread)
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Test/simulate_redis_state.cpp")
    add_executable(simulate_redis_state ../Test/simulate_redis_state.cpp)
    target_include_directories(simulate_redis_state PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)
    if(WIN32)
        target_link_libraries(simulate_redis_state ws2_32)
    else()
        target_link_libraries(simulate_redis_state pthread)
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Test/test_env_and_db_uri.cpp")
    add_executable(test_env_and_db_uri ../Test/test_env_and_db_uri.cpp src/backend/Database.cpp)
    target_include_directories(test_env_and_db_uri PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)
    if(WIN32)
        target_link_libraries(test_env_and_db_uri PRIVATE sqlite3_static ws2_32)
    else()
        target_link_libraries(test_env_and_db_uri PRIVATE sqlite3_static pthread ${CMAKE_DL_LIBS})
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Test/simulate_fetch.cpp")
    add_executable(simulate_fetch ../Test/simulate_fetch.cpp)
    target_include_directories(simulate_fetch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)
    if(WIN32)
        target_link_libraries(simulate_fetch ws2_32 winhttp)
    else()
        target_link_libraries(simulate_fetch pthread)
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Test/simulate_graph.cpp")
    add_executable(simulate_graph ../Test/simulate_graph.cpp)
    target_include_directories(simulate_graph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/novacpp)
    if(WIN32)
        target_link_libraries(simulate_graph ws2_32)
    else()
        target_link_libraries(simulate_graph pthread)
    endif()
endif()

# Custom target to open the HTML file using the default browser
if(WIN32)
    add_custom_target(run-html
        COMMAND cmd.exe /c start "" "http://localhost:8080"
        COMMENT "Opening http://localhost:8080 in the default browser..."
    )
endif()
