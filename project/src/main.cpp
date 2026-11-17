#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <print>
#include <stdexcept>
#include <string>

#include "agent.h"
#include "alert_sink.h"
#include "edr_handle.h"
#include "field_traits.h"
#include "file_source.h"
#include "os_source.h"

namespace {

struct Config {
    std::string path;
    std::string out;
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
        } else if (arg == "--out") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("после --out нет значения");
            }
            config.out = argv[++i];
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
            "использование: nano-edr [--quiet] [--out <файл>] "
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
        nano_edr::EdrHandle edr(config.path);

        const std::string text = edr.Config("dedup_period_ms", "10000");
        uint64_t dedup_period_ms = 0;
        if (!nano_edr::FieldTraits<uint64_t>::Parse(text, &dedup_period_ms)) {
            std::print(stderr, "[WARN] dedup_period_ms не разбирается: {}\n", text);
            dedup_period_ms = 10000;
        }
        nano_edr::Agent agent(config.is_quiet, dedup_period_ms);

        if (config.use_file) {
            source = std::make_unique<nano_edr::FileSource>(config.path);
        } else {
            source = std::make_unique<nano_edr::OsSource>(config.path);
        }
        nano_edr::FanOutSink targets;
        targets.Add(nano_edr::ConsoleSink());
        targets.Add(nano_edr::EdrSink(&edr));
        if (!config.out.empty()) {
            targets.Add(nano_edr::FileSink(config.out));
        }
        agent.Subscribe(std::move(targets));
        source->Run(&agent);
        if (edr.Flush() != EDR_OK) {
            std::print(stderr, "[WARN] детекты не отправлены\n");
        }
        agent.PrintSummary();
    } catch (const std::exception& e) {
        std::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
    return 0;
}