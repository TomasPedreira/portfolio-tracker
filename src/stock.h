#ifndef STOCK_H
#define STOCK_H

#include <string>

class Stock {
public:
    Stock(std::string symbol, std::string name, double current_price);

    const std::string& symbol() const;
    const std::string& name() const;
    double current_price() const;

    void set_current_price(double current_price);

private:
    std::string symbol_;
    std::string name_;
    double current_price_;
};

#endif
