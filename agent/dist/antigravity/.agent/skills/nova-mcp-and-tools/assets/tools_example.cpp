#include "novacpp/nova.hpp"
#include <iostream>

void demonstrateToolRegistry() {
    np::ToolRegistry registry;

    // 1. Register parameter-typed tool
    registry.registerTool(
        "query_database",
        "Executes a read-only SQL SELECT query against the local SQLite database",
        {
            {"query", "string", "The SQL SELECT statement to execute", true, {}},
            {"limit", "integer", "Maximum number of rows to return", false, {}}
        },
        [](const std::unordered_map<std::string, std::string>& args) {
            std::string q = args.at("query");
            std::string limit = "10";
            auto it = args.find("limit");
            if (it != args.end()) limit = it->second;

            return "{\"status\":\"ok\",\"query\":\"" + q + "\",\"limit\":" + limit + ",\"rows\":[{\"id\":1}]}";
        }
    );

    // 2. Inspect OpenAI schema JSON
    auto tool = registry.getTool("query_database");
    if (tool) {
        std::cout << "--- OPENAI FUNCTION JSON SCHEMA ---\n"
                  << tool->schema().toOpenAISchemaJSON() << "\n\n";
    }

    // 3. Simulate parsing LLM response arguments
    std::string llmOutputArgs = "{\"query\":\"SELECT * FROM metrics\",\"limit\":5}";
    auto parsedArgs = np::parseToolArguments(llmOutputArgs);

    // 4. Execute tool
    std::string result = tool->execute(parsedArgs);
    std::cout << "--- TOOL EXECUTION RESULT ---\n" << result << "\n";
}

int main() {
    demonstrateToolRegistry();
    return 0;
}

