#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <string>

struct AppConfig {
    std::string last_file;
};

namespace AppConfigIO {
    AppConfig load();
    void save(const AppConfig& config);
}

#endif
