cmake_minimum_required(VERSION 3.10)
project(NovaCPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(NovaCPP src/main.cpp src/App.cpp)

if(WIN32)
    target_link_libraries(NovaCPP ws2_32)
endif()

# Custom target to open the HTML file using the default browser
add_custom_target(run-html
    COMMAND cmd.exe /c start "" "http://localhost:8080"
    COMMENT "Opening http://localhost:8080 in the default browser..."
)
