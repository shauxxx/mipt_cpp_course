#include "agent.h"
#include "file_source.h"
#include "os_source.h"
#include <charconv>
#include <cstring>
#include <print>
#include <stdexcept>
#include <string>

struct Config {
    bool is_quiet = false;
    bool use_file_source = false;
    long long window_size = 64;
    std::string path;
};

void ParseArgs(int argc, char** argv, Config& cfg) {
    if (argc < 2) {
        throw std::invalid_argument("использование: nano-edr [--file <путь>|<путь>] [--quiet] [--window-size <N>]");
    }

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        
        if (a == "--quiet") {
            cfg.is_quiet = true;
            
        } else if (a == "--window-size") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("после --window-size нет значения");
            }
            const char* s = argv[++i];
            long long n = 0;
            auto [p, ec] = std::from_chars(s, s + std::strlen(s), n);
            if (ec == std::errc() && n >= 0) {
                cfg.window_size = n;
            } else {
                throw std::invalid_argument("неверное значение для --window-size: '" + std::string(s) + "'");
            }
            
        } else if (a == "--file") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("после --file нет значения");
            }
            cfg.use_file_source = true;
            if (!cfg.path.empty()) {
                throw std::invalid_argument("путь задан дважды: '" + cfg.path + "' и '" + std::string(argv[i + 1]) + "'");
            }
            cfg.path = argv[++i];
            
        } else if (!a.empty() && a[0] != '-') {
            if (!cfg.path.empty()) {
                throw std::invalid_argument("путь задан дважды: '" + cfg.path + "' и '" + a + "'");
            }
            cfg.path = a;
            
        } else {
            throw std::invalid_argument("неизвестная опция: '" + a + "'");
        }
    }

    if (cfg.path.empty()) {
        throw std::invalid_argument("не указан путь к журналу");
    }
}

int main(int argc, char** argv) {
    Config cfg;

    try {
        ParseArgs(argc, argv, cfg);
    } catch (const std::exception& e) {
        std::print(stderr, "Error: {}\n", e.what());
        return 2;
    }

    nano_edr::Agent agent(static_cast<std::size_t>(cfg.window_size), cfg.is_quiet);

    try {
        if (cfg.use_file_source) {
            nano_edr::FileSource source(cfg.path);
            source.Run(&agent);
        } else {
            nano_edr::OsSource source(cfg.path);
            source.Run(&agent);
        }

        agent.PrintSummary();

    } catch (const std::exception& e) {
        std::print(stderr, "Error: {}\n", e.what());
        return 1;
    }

    return 0;
}