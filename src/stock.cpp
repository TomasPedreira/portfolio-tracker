#include "stock.h"

#include <utility>

Stock::Stock(std::string symbol, std::string name, double current_price)
    : symbol_(std::move(symbol)), name_(std::move(name)), current_price_(current_price) {}

const std::string& Stock::symbol() const {
    return symbol_;
}

const std::string& Stock::name() const {
    return name_;
}

double Stock::current_price() const {
    return current_price_;
}

void Stock::set_current_price(double current_price) {
    current_price_ = current_price;
}
