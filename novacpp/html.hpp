#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace np {

class NovaBuilder {
private:
    std::vector<std::string> elements;

public:
    // Clears the DOM (used when a re-render is triggered by a state change)
    void clear() {
        elements.clear();
    }

    // Note: 'return' is a reserved keyword in C++, so we can't use `np.return()`.
    // I used `render()` instead, which serves the exact purpose you described!
    void render(const std::string& html_string) {
        elements.push_back(html_string);
    }

    // This method compiles everything into the final index.html
    void compile(const std::string& output_path) {
        std::ofstream file(output_path);
        if (!file.is_open()) {
            std::cerr << "NovaCPP Error: Could not open " << output_path << " for writing.\n";
            return;
        }

        // Write the basic HTML wrapper
        file << "<!DOCTYPE html>\n";
        file << "<html lang=\"en\">\n<head>\n";
        file << "    <meta charset=\"UTF-8\">\n";
        file << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        file << "    <title>NovaCPP App</title>\n";
        file << "    <link rel=\"stylesheet\" href=\"styles.css\">\n";
        file << "</head>\n<body>\n";
        file << "    <div id=\"root\">\n";

        // Add everything line by line exactly as added in the src files
        for (const auto& el : elements) {
            file << "        " << el << "\n";
        }

        // Close the wrapper
        file << "    </div>\n";
        file << "    <script src=\"app.js\"></script>\n";
        file << "</body>\n</html>\n";
        
        file.close();
        std::cout << "NovaCPP: Successfully built UI to " << output_path << "!\n";
    }
};

} // namespace np
