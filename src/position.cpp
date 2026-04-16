#include "position.h"

#include <utility>

Position::Position(Stock stock, double buy_price, std::string acquisition_date, double amount)
    : stock_(std::move(stock)),
      buy_price_(buy_price),
      acquisition_date_(std::move(acquisition_date)),
      amount_(amount) {}

const Stock& Position::stock() const {
    return stock_;
}

double Position::buy_price() const {
    return buy_price_;
}

const std::string& Position::acquisition_date() const {
    return acquisition_date_;
}

double Position::amount() const {
    return amount_;
}

double Position::cost_basis() const {
    return buy_price_ * amount_;
}

double Position::current_value() const {
    return stock_.current_price() * amount_;
}

double Position::gain_loss() const {
    return current_value() - cost_basis();
}

double Position::gain_loss_percent() const {
    if (cost_basis() == 0.0) {
        return 0.0;
    }

    return gain_loss() / cost_basis() * 100.0;
}

void Position::update_stock_price(double price) {
    stock_.set_current_price(price);
}

void Position::set_buy_price(double buy_price) {
    buy_price_ = buy_price;
}

void Position::set_acquisition_date(std::string acquisition_date) {
    acquisition_date_ = std::move(acquisition_date);
}

void Position::set_amount(double amount) {
    amount_ = amount;
}
