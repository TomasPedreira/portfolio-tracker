#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include "position.h"

#include <vector>

class Portfolio {
public:
    void add_position(Position position);

    const std::vector<Position>& positions() const;

    double total_cost_basis() const;
    double total_current_value() const;
    double total_gain_loss() const;
    double total_gain_loss_percent() const;

private:
    std::vector<Position> positions_;
};

#endif
