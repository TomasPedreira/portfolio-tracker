#include "sample_data.h"

#include "position.h"
#include "stock.h"

Portfolio SampleData::make_portfolio() {
    Portfolio portfolio;

    portfolio.add_position(Position(Stock("AAPL", "Apple Inc.", 198.42), 175.30, "2025-03-14", 8.0));
    portfolio.add_position(Position(Stock("MSFT", "Microsoft Corporation", 421.18), 389.75, "2024-11-02", 5.0));
    portfolio.add_position(Position(Stock("VWCE", "Vanguard FTSE All-World UCITS ETF", 132.64), 118.20, "2023-07-21", 14.0));
    portfolio.add_position(Position(Stock("NVDA", "NVIDIA Corporation", 112.76), 126.40, "2025-01-09", 6.0));
    portfolio.add_position(Position(Stock("GOOGL", "Alphabet Inc.", 161.30), 143.90, "2024-04-18", 4.0));
    portfolio.add_position(Position(Stock("ASML", "ASML Holding N.V.", 891.20), 742.10, "2023-10-06", 2.0));
    portfolio.add_position(Position(Stock("TSLA", "Tesla Inc.", 178.58), 221.00, "2025-05-30", 3.0));
    portfolio.add_position(Position(Stock("IWDA", "iShares Core MSCI World UCITS ETF", 101.72), 93.40, "2022-12-12", 20.0));

    return portfolio;
}
