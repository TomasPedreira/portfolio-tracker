#ifndef POSITION_H
#define POSITION_H

#include "stock.h"

#include <string>

class Position {
public:
    Position(Stock stock, double buy_price, std::string acquisition_date, double amount);

    const Stock& stock() const;
    double buy_price() const;
    const std::string& acquisition_date() const;
    double amount() const;

    double cost_basis() const;
    double current_value() const;
    double gain_loss() const;
    double gain_loss_percent() const;

private:
    Stock stock_;
    double buy_price_;
    std::string acquisition_date_;
    double amount_;
};

#endif
