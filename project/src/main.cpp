#include <cstddef>
#include <cstdio>
#include <exception>
#include <print>
#include <stdexcept>
#include <string>

#include "agent.h"
#include "file_source.h"
#include "os_source.h"

namespace {

struct Config {
    std::string path;
    bool use_file = false;
    bool is_quiet = false;
};

Config ParseArgs(int argc, char** argv) {
    Config config;
    bool has_path = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--quiet") {
            config.is_quiet = true;
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
            "использование: nano-edr [--quiet] "
            "(<журнал> | --file <журнал>)");
    }
    return config;
}

}  // namespace

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
        nano_edr::Agent agent(config.is_quiet);
        if (config.use_file) {
            source = std::make_unique<nano_edr::FileSource>(config.path);
        } else {
            source = std::make_unique<nano_edr::OsSource>(config.path);
        }
        source->Run(&agent);
        agent.PrintSummary();
    } catch (const std::exception& e) {
        std::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
    return 0;
}