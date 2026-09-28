#include <charconv>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <print>
#include <string>
#include <vector>

#include "agent_rules.h"
#include "event_list.h"
#include "fields.h"
#include "parse.h"
#include "rules.h"

struct Config {
    bool is_quiet = false;
    long long window_size = 64;
    std::ifstream log;
};

void ParseArgs(int argc, char** argv, bool& is_quiet, long long& window_size, std::ifstream& log) {
    if (argc < 2) {
        throw std::invalid_argument("использование: nano-edr <журнал.log>\n");
    }

    log.open(argv[1]);
    if (!log) {
        throw std::invalid_argument(std::string("не удалось открыть журнал: ") + argv[1]);
    }

    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--quiet") {
            is_quiet = true;
        } else if (a == "--window-size" && i + 1 < argc) {
            const char* s = argv[i + 1];
            long long n = 0;
            auto [p, ec] = std::from_chars(s, s + std::strlen(s), n);
            if (ec == std::errc() && n >= 0) {
                window_size = n;
                ++i;
            }
        }
    }
}

void OutPutContext(const nano_edr::EventList& window) {
    size_t total = window.size;
    if (total == 0) {
        return;
    }
    if (total == 1) {
        std::print("[CTX] -1: ts={} type={} pid={}\n", window.tail->event.ts, window.tail->event.type, window.tail->event.pid);
    } else {
        nano_edr::EventNode* cur = window.head;
        for (size_t k = 0; k < total; k++) {
            if (k >= total - 2) {
                std::print("[CTX] -{}: ts={} type={} pid={}\n", total - k, cur->event.ts, cur->event.type, cur->event.pid);
            }
            cur = cur->next;
        }
    }
}

void FinalPrint(long long lines, long long comments, const std::map<std::string, long long>& types) {
    std::print("строк {}, из них комментариев {}\n", lines, comments);
    std::print("событий всего : {}\n", lines - comments);
    for (auto& [type, count] : types)
        std::print("событий типа {} всего : {}\n", type, count);
}

void Execution(Config& config) {
    nano_edr::EventList window;
    window.capacity = config.window_size;

    long long lines = 0;
    long long comments = 0;

    std::map<std::string, long long> event_types_count;
    std::string line;

    const nano_edr::Rule* rules = nano_edr::AgentRules();
    const size_t rule_count = nano_edr::AgentRuleCount();

    while (std::getline(config.log, line)) {
        ++lines;

        if (nano_edr::IsBlankOrComment(&line)) {
            if (!line.empty()) ++comments;
            continue;
        }

        nano_edr::Event event;
        if (!nano_edr::ParseEventLine(&line, &event)) {
            continue;
        }
        ++event_types_count[event.type];

        size_t detected = nano_edr::CheckRules(event, rules, rule_count);

        if (detected && !config.is_quiet) {
            OutPutContext(window);
        }

        ListPushBack(&window, &event);
    }

    if (!config.is_quiet) {
        FinalPrint(lines, comments, event_types_count);
    }
}

int main(int argc, char** argv) {
    Config cur_conf;
    try {
        ParseArgs(argc, argv, cur_conf.is_quiet, cur_conf.window_size, cur_conf.log);
    } catch (std::exception& e) {
        std::print(stderr, "Error: {}\n", e.what());
        return 2;
    }

    try {
        Execution(cur_conf);
    } catch (std::exception& e) {
        std::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
    return 0;
}
