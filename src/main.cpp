#include "../novacpp/html.hpp"

// Forward declare the function from App.cpp
void renderApp(np::NovaBuilder& np);

int main() {
    // 1. Create the instance of our framework class
    np::NovaBuilder np;
    
    // 2. Start the Live Server on port 8080!
    // We pass renderApp so the server can automatically re-render 
    // the UI whenever a button triggers a state change.
    np.listen(8080, renderApp);
    
    return 0;
}
