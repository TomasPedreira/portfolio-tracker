#include "portfolio.h"

#include <utility>

void Portfolio::add_position(Position position) {
    positions_.push_back(std::move(position));
}

const std::vector<Position>& Portfolio::positions() const {
    return positions_;
}

double Portfolio::total_cost_basis() const {
    double total = 0.0;

    for (const Position& position : positions_) {
        total += position.cost_basis();
    }

    return total;
}

double Portfolio::total_current_value() const {
    double total = 0.0;

    for (const Position& position : positions_) {
        total += position.current_value();
    }

    return total;
}

double Portfolio::total_gain_loss() const {
    return total_current_value() - total_cost_basis();
}

double Portfolio::total_gain_loss_percent() const {
    if (total_cost_basis() == 0.0) {
        return 0.0;
    }

    return total_gain_loss() / total_cost_basis() * 100.0;
}
