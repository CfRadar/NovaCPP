#include "../novacpp/html.hpp"

// Forward declare the function from App.cpp
void renderApp(np::NovaBuilder& np);

int main() {
    // 1. Create the instance of our framework class
    np::NovaBuilder np;
    
    // 2. Call our App code to build the Virtual DOM / HTML list
    renderApp(np);
    
    // 3. Compile it and output to render/index.html
    // Ensure the render directory exists or is created by cmake
    np.compile("render/index.html");
    
    return 0;
}
