#include "app_config.h"

#include <fstream>
#include <string>

static const char* kConfigPath = "portfolio_tracker.cfg";

AppConfig AppConfigIO::load() {
    AppConfig config;
    std::ifstream f(kConfigPath);
    if (!f.is_open()) return config;

    std::string line;
    while (std::getline(f, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);
        if (key == "file") config.last_file = val;
    }
    return config;
}

void AppConfigIO::save(const AppConfig& config) {
    std::ofstream f(kConfigPath);
    if (!f.is_open()) return;
    f << "file=" << config.last_file << '\n';
}
