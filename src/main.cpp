#include "portfolio_app.h"

int main() {
    PortfolioApp app;

    while (!app.should_close()) {
        app.input();
        app.update();
        app.render();
    }

    return 0;
}
