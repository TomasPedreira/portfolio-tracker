#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include "position.h"
#include "stock.h"

#include <cstddef>
#include <string>
#include <vector>

class Portfolio {
public:
    // Positions
    void add_position(Position position);
    bool remove_position_at(std::size_t index);
    bool update_position_amount(std::size_t index, double amount);
    bool update_position_buy_price(std::size_t index, double buy_price);
    bool update_position_date(std::size_t index, std::string date);
    const std::vector<Position>& positions() const;

    // Stock registry — shared prices across all positions
    void add_stock(Stock stock);
    bool remove_stock_at(std::size_t index);
    bool has_stock(const std::string& symbol) const;
    void update_stock_price(const std::string& symbol, double price);
    const std::vector<Stock>& stocks() const;

    std::vector<std::size_t> position_indices_for(const std::string& symbol) const;

    // Totals
    double total_cost_basis() const;
    double total_current_value() const;
    double total_gain_loss() const;
    double total_gain_loss_percent() const;

private:
    std::vector<Stock>    stocks_;
    std::vector<Position> positions_;
};

#endif
