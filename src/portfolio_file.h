#ifndef PORTFOLIO_FILE_H
#define PORTFOLIO_FILE_H

#include "portfolio.h"

#include <string>

namespace PortfolioFile {
    // Returns an empty portfolio if the file doesn't exist.
    Portfolio load(const std::string& path);

    // Overwrites the file with the current portfolio state.
    void save(const Portfolio& portfolio, const std::string& path);

    // Reads only the [stocks] section and updates prices in the given portfolio.
    // Positions and UI state are untouched. Returns false if the file can't be read.
    bool load_stock_prices_only(const std::string& path, Portfolio& into);
}

#endif
