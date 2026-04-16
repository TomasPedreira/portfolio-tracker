#ifndef PORTFOLIO_FILE_H
#define PORTFOLIO_FILE_H

#include "portfolio.h"

#include <string>

namespace PortfolioFile {
    // Returns an empty portfolio if the file doesn't exist.
    Portfolio load(const std::string& path);

    // Overwrites the file with the current portfolio state.
    void save(const Portfolio& portfolio, const std::string& path);
}

#endif
