#include <charconv>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <print>
#include <stdexcept>
#include <string>
#include <system_error>

#include "agent.h"
#include "file_source.h"
#include "os_source.h"

namespace {

struct Config {
    std::string path;
    bool use_file = false;
    bool is_quiet = false;
    std::size_t window_size = 64;
};

Config ParseArgs(int argc, char** argv) {
    Config config;
    bool has_path = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--quiet") {
            config.is_quiet = true;
        } else if (arg == "--window-size") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("после --window-size нет значения");
            }
            const char* text = argv[++i];
            std::size_t value = 0;
            const auto [ptr, ec] =
                std::from_chars(text, text + std::strlen(text), value);
            if (ec != std::errc() || *ptr != '\0') {
                throw std::invalid_argument(
                    std::string("неверный размер окна: ") + text);
            }
            config.window_size = value;
        } else if (arg == "--file") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("после --file нет значения");
            }
            if (has_path) {
                throw std::invalid_argument("путь к журналу задан дважды");
            }
            config.path = argv[++i];
            config.use_file = true;
            has_path = true;
        } else if (arg.starts_with("--")) {
            throw std::invalid_argument("неизвестная опция: " + arg);
        } else {
            if (has_path) {
                throw std::invalid_argument("путь к журналу задан дважды");
            }
            config.path = arg;
            has_path = true;
        }
    }

    if (!has_path) {
        throw std::invalid_argument(
            "использование: nano-edr [--quiet] [--window-size N] "
            "(<журнал> | --file <журнал>)");
    }
    return config;
}

}

int main(int argc, char** argv) {
    Config config;
    try {
        config = ParseArgs(argc, argv);
    } catch (const std::exception& e) {
        std::print(stderr, "Error: {}\n", e.what());
        return 2;
    }

    try {
        std::unique_ptr<nano_edr::IEventSource> source;
        nano_edr::Agent agent(config.window_size, config.is_quiet);
        if (config.use_file) {
            source = std::make_unique<nano_edr::FileSource>(config.path);
        } else {
            source = std::make_unique<nano_edr::OsSource>(config.path);
        }
        source->Run(&agent);
    } catch (const std::exception& e) {
        std::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
    return 0;
}