#include "novacpp/nova.hpp"
#include <memory>

class SafeStaticCard : public np::Component {
public:
    SafeStaticCard() : Component("safe-static-card") {}
    void render(np::RenderContext& np) override {
        np << "<div class='bento-card col-6'><h3>Static Card</h3><p>Safe lifetime.</p></div>";
    }
};

class SafeDynamicCard : public np::Component {
private:
    std::string m_userId;
public:
    SafeDynamicCard(std::string id) : Component("dynamic-card-" + id), m_userId(std::move(id)) {}
    void render(np::RenderContext& np) override {
        np << "<div class='bento-card col-6'><h3>User: " + m_userId + "</h3></div>";
    }
};

static SafeStaticCard g_staticCard;

void renderDemoPage(np::RenderContext& np) {
    np << "<div class='bento-grid'>";
    
    // 1. Static global component
    np.renderComponent(g_staticCard);

    // 2. Heap-allocated shared component with server ownership
    auto dynamicUser = std::make_shared<SafeDynamicCard>("alice-42");
    np.renderComponent(dynamicUser);

    np << "</div>";
}

int main() {
    np::NovaBuilder app;
    app.route("/", renderDemoPage);
    return 0;
}

