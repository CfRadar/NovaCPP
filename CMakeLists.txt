cmake_minimum_required(VERSION 3.10)
project(NovaCPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(NovaCPP src/main.cpp src/App.cpp)

# Custom target to open the HTML file using the default browser
add_custom_target(run-html
    COMMAND cmd.exe /c start "" "${CMAKE_SOURCE_DIR}/render/index.html"
    COMMENT "Opening index.html in the default browser..."
)
