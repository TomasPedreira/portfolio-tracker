#include "portfolio_app.h"
#include "app_config.h"
#include "portfolio_file.h"

#include <iostream>
#include <string>

namespace {
struct LaunchOptions {
    std::string file_path;
    bool        should_exit = false;
    int         exit_code   = 0;
};

void print_usage(const char* exe) {
    std::cout << "Portfolio Tracker\n\n"
              << "Usage:\n"
              << "  " << exe << "\n"
              << "  " << exe << " --file <path>\n"
              << "  " << exe << " --help\n\n"
              << "Options:\n"
              << "  --file <path>   Load positions from a TSV file on startup and save\n"
              << "                  back to it whenever a position is added in the GUI.\n"
              << "                  Creates the file if it does not exist yet.\n";
}

LaunchOptions parse_launch_options(int argc, char* argv[]) {
    LaunchOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help") {
            print_usage(argv[0]);
            options.should_exit = true;
            return options;
        }

        if (arg == "--file") {
            if (i + 1 >= argc) {
                std::cerr << "--file requires a path argument.\n\n";
                print_usage(argv[0]);
                options.should_exit = true;
                options.exit_code   = 1;
                return options;
            }
            options.file_path = argv[++i];
            continue;
        }

        std::cerr << "Unknown argument: " << arg << "\n\n";
        print_usage(argv[0]);
        options.should_exit = true;
        options.exit_code   = 1;
        return options;
    }

    return options;
}

Portfolio make_starting_portfolio(const LaunchOptions& options) {
    Portfolio portfolio;

    if (!options.file_path.empty())
        portfolio = PortfolioFile::load(options.file_path);

    return portfolio;
}
}

int main(int argc, char* argv[]) {
    LaunchOptions options = parse_launch_options(argc, argv);

    if (options.should_exit)
        return options.exit_code;

    // If no --file was given, check the config for the last used file.
    const AppConfig config = AppConfigIO::load();
    if (options.file_path.empty() && !config.last_file.empty())
        options.file_path = config.last_file;

    // If --file was given explicitly, persist it as the new last file.
    if (!options.file_path.empty() && options.file_path != config.last_file) {
        AppConfig updated;
        updated.last_file = options.file_path;
        AppConfigIO::save(updated);
    }

    PortfolioApp app(make_starting_portfolio(options), options.file_path);

    while (!app.should_close()) {
        app.input();
        app.update();
        app.render();
    }

    return 0;
}
