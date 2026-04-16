#ifndef PORTFOLIO_APP_H
#define PORTFOLIO_APP_H

#include "portfolio.h"
#include "portfolio_renderer.h"

#include <string>

class PortfolioApp {
public:
    explicit PortfolioApp(Portfolio portfolio, std::string file_path = {});
    ~PortfolioApp();

    bool should_close() const;
    void input();
    void update();
    void render() const;

private:
    void try_submit();
    void try_save_as();
    void try_add_stock();
    void try_confirm_price();
    void try_confirm_position_edit();
    bool handle_stock_panel_click(const Vector2& mouse, int screen_w, int screen_h);
    bool handle_positions_table_click(const Vector2& mouse, int screen_w, int screen_h);

    Portfolio         portfolio_;
    PortfolioRenderer renderer_;
    float             scroll_offset_;
    AddPanelState     panel_;
    SavePopupState    save_popup_;
    StockPanelState   stock_panel_;
    PositionTableState position_table_;
    float             backspace_timer_;
    std::string       file_path_;
};

#endif
