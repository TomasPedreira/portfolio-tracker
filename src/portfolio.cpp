#include "portfolio.h"

#include <algorithm>
#include <utility>

// ── Positions ────────────────────────────────────────────────────────────────

void Portfolio::add_position(Position position) {
    // Ensure the stock is in the registry (but don't overwrite an existing entry's price)
    if (!has_stock(position.stock().symbol()))
        stocks_.push_back(position.stock());
    positions_.push_back(std::move(position));
}

bool Portfolio::remove_position_at(std::size_t index) {
    if (index >= positions_.size()) return false;
    positions_.erase(positions_.begin() + static_cast<long long>(index));
    return true;
}

bool Portfolio::update_position_amount(std::size_t index, double amount) {
    if (index >= positions_.size()) return false;
    positions_[index].set_amount(amount);
    return true;
}

bool Portfolio::update_position_buy_price(std::size_t index, double buy_price) {
    if (index >= positions_.size()) return false;
    positions_[index].set_buy_price(buy_price);
    return true;
}

bool Portfolio::update_position_date(std::size_t index, std::string date) {
    if (index >= positions_.size()) return false;
    positions_[index].set_acquisition_date(std::move(date));
    return true;
}

const std::vector<Position>& Portfolio::positions() const {
    return positions_;
}

// ── Stock registry ────────────────────────────────────────────────────────────

void Portfolio::add_stock(Stock stock) {
    for (auto& s : stocks_) {
        if (s.symbol() == stock.symbol()) {
            s.set_current_price(stock.current_price());
            return;
        }
    }
    stocks_.push_back(std::move(stock));
}

bool Portfolio::remove_stock_at(std::size_t index) {
    if (index >= stocks_.size()) return false;
    const std::string symbol = stocks_[index].symbol();
    stocks_.erase(stocks_.begin() + static_cast<long long>(index));

    positions_.erase(
        std::remove_if(positions_.begin(), positions_.end(),
                       [&](const Position& p) { return p.stock().symbol() == symbol; }),
        positions_.end());

    return true;
}

bool Portfolio::has_stock(const std::string& symbol) const {
    for (const auto& s : stocks_)
        if (s.symbol() == symbol) return true;
    return false;
}

void Portfolio::update_stock_price(const std::string& symbol, double price) {
    for (auto& s : stocks_) {
        if (s.symbol() == symbol) { s.set_current_price(price); break; }
    }
    for (auto& p : positions_) {
        if (p.stock().symbol() == symbol)
            p.update_stock_price(price);
    }
}

const std::vector<Stock>& Portfolio::stocks() const {
    return stocks_;
}

// ── Totals ────────────────────────────────────────────────────────────────────

double Portfolio::total_cost_basis() const {
    double total = 0.0;
    for (const auto& p : positions_) total += p.cost_basis();
    return total;
}

double Portfolio::total_current_value() const {
    double total = 0.0;
    for (const auto& p : positions_) total += p.current_value();
    return total;
}

double Portfolio::total_gain_loss() const {
    return total_current_value() - total_cost_basis();
}

double Portfolio::total_gain_loss_percent() const {
    if (total_cost_basis() == 0.0) return 0.0;
    return total_gain_loss() / total_cost_basis() * 100.0;
}
