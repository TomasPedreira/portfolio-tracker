#ifndef PORTFOLIO_APP_H
#define PORTFOLIO_APP_H

#include "portfolio.h"
#include "portfolio_renderer.h"

class PortfolioApp {
public:
    PortfolioApp();
    ~PortfolioApp();

    bool should_close() const;
    void input();
    void update();
    void render() const;

private:
    Portfolio portfolio_;
    PortfolioRenderer renderer_;
    float scroll_offset_;
};

#endif
