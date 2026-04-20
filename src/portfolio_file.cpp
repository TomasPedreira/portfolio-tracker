#include "portfolio_file.h"

#include "position.h"
#include "stock.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::vector<std::string> split_tab(const std::string& line) {
    std::vector<std::string> fields;
    std::istringstream ss(line);
    std::string field;
    while (std::getline(ss, field, '\t'))
        fields.push_back(field);
    return fields;
}
}

Portfolio PortfolioFile::load(const std::string& path) {
    Portfolio portfolio;
    std::ifstream file(path);
    if (!file.is_open()) return portfolio;

    // Two-pass: detect format (new = has "[stocks]" marker, old = plain header)
    enum class Section { None, Stocks, Positions };
    Section section = Section::None;
    bool new_format = false;

    // First check for section markers
    {
        std::string line;
        while (std::getline(file, line)) {
            if (line == "[stocks]" || line == "[positions]") { new_format = true; break; }
        }
    }
    file.clear();
    file.seekg(0);

    if (new_format) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            if (line == "[stocks]")    { section = Section::Stocks;    continue; }
            if (line == "[positions]") { section = Section::Positions; continue; }
            if (line.rfind("symbol", 0) == 0) continue;  // skip header rows

            const auto f = split_tab(line);
            if (section == Section::Stocks && f.size() >= 3) {
                try {
                    portfolio.add_stock(Stock(f[0], f[1], std::stod(f[2])));
                } catch (...) {}
            } else if (section == Section::Positions && f.size() >= 5) {
                try {
                    const std::string& sym  = f[0];
                    const std::string& name = f[1];
                    double amount    = std::stod(f[2]);
                    double buy_price = std::stod(f[3]);
                    // f[4] = value (computed, skip)
                    const std::string& date = f[5];

                    // Current price comes from registry if the stock was already loaded
                    double cur_price = buy_price;
                    if (portfolio.has_stock(sym)) {
                        for (const auto& s : portfolio.stocks())
                            if (s.symbol() == sym) { cur_price = s.current_price(); break; }
                    }
                    Stock    stock(sym, name, cur_price);
                    Position pos(std::move(stock), buy_price, date, amount);
                    portfolio.add_position(std::move(pos));
                } catch (...) {}
            }
        }
    } else {
        // Old format: plain header + data rows, infer current_price = buy_price
        std::string line;
        bool header = true;
        while (std::getline(file, line)) {
            if (header) { header = false; continue; }
            if (line.empty()) continue;
            const auto f = split_tab(line);
            if (f.size() < 6) continue;
            try {
                double amount    = std::stod(f[2]);
                double buy_price = std::stod(f[3]);
                Stock    stock(f[0], f[1], buy_price);
                Position pos(std::move(stock), buy_price, f[5], amount);
                portfolio.add_position(std::move(pos));
            } catch (...) {}
        }
    }

    return portfolio;
}

bool PortfolioFile::load_stock_prices_only(const std::string& path, Portfolio& into) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    enum class Section { None, Stocks, Other };
    Section section = Section::None;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line == "[stocks]")    { section = Section::Stocks; continue; }
        if (line[0] == '[')        { section = Section::Other;  continue; }
        if (section != Section::Stocks) continue;
        if (line.rfind("symbol", 0) == 0) continue;

        const auto f = split_tab(line);
        if (f.size() < 3) continue;
        try {
            into.update_stock_price(f[0], std::stod(f[2]));
        } catch (...) {}
    }
    return true;
}

void PortfolioFile::save(const Portfolio& portfolio, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return;

    file << std::fixed << std::setprecision(10);

    // Stocks section
    file << "[stocks]\n";
    file << "symbol\tname\tcurrent_price\n";
    for (const Stock& s : portfolio.stocks())
        file << s.symbol() << '\t' << s.name() << '\t' << s.current_price() << '\n';

    file << '\n';

    // Positions section
    file << "[positions]\n";
    file << "symbol\tname\tamount\tbuy_price\tvalue\tdate\n";
    for (const Position& pos : portfolio.positions()) {
        file << pos.stock().symbol()   << '\t'
             << pos.stock().name()     << '\t'
             << pos.amount()           << '\t'
             << pos.buy_price()        << '\t'
             << pos.cost_basis()       << '\t'
             << pos.acquisition_date() << '\n';
    }
}
