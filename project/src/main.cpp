#include <charconv>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <print>
#include <string>
#include <unordered_map>
#include <vector>

#include "../kit/include/l1.2/parse.h"
#include "../kit/include/l1.2/event_list.h"

int main(int argc, char** argv) {
    const std::vector<std::string> markers = {
        "wscript.exe",
        ".locked",
        "certutil.exe",
        "\\Startup\\",
    };

    const long long default_window = 64;
    long long window_size = default_window;
    bool silent_mode = false;

    if (argc < 2) {
        std::print(stderr, "использование: nano-edr <журнал.log>\n");
        return 2;
    }

    std::ifstream log(argv[1]);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", argv[1]);
        return 2;
    }

 


    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--quiet") {
            silent_mode = true;
            continue;
        }

        if (arg == "--window-size" && i + 1 < argc) {
            const char* raw = argv[i + 1];
            long long parsed = 0;
            const auto [end, err] =
                std::from_chars(raw, raw + std::strlen(raw), parsed);
            if (err == std::errc() && parsed >= 0) {
                window_size = parsed;
                ++i;
            }
        }
    }


    nano_edr::EventList window{};
    window.head     = nullptr;
    window.tail     = nullptr;
    window.size     = 0;
    window.capacity = static_cast<std::size_t>(window_size);

    long long total_lines    = 0;
    long long comment_lines  = 0;
    long long parsed_events  = 0;

    std::unordered_map<std::string, long long> type_tally;
    std::string current;

    while (std::getline(log, current)) {
        ++total_lines;

        if (nano_edr::IsBlankOrComment(&current)) {
            if (!current.empty()) {
                ++comment_lines;
            }
            continue;
        }


        bool flagged = false;
        for (const auto& marker : markers) {
            if (current.find(marker) != std::string::npos) {
                std::print("[DETECT] строка {}, признак {}: {}\n",
                           total_lines, marker, current);
                flagged = true;
            }
        }


        nano_edr::Event parsed{};
        if (!nano_edr::ParseEventLine(&current, &parsed)) {
            continue;
        }

        ++parsed_events;
        ++type_tally[parsed.type];


        if (flagged && !silent_mode) {
            std::vector<const nano_edr::Event*> snapshot;
            for (nano_edr::EventNode* cursor = window.head; cursor;
                 cursor = cursor->next) {
                snapshot.push_back(&cursor->event);
            }

            const int count = static_cast<int>(snapshot.size());
            const int first = count > 2 ? count - 2 : 0;

            for (int idx = first; idx < count; ++idx) {
                const nano_edr::Event* entry = snapshot[idx];
                std::print("[CTX] {}: ts={} type={} pid={}\n",
                           idx - count, entry->ts, entry->type, entry->pid);
            }
        }

        ListPushBack(&window, &parsed);
    }

    if (!silent_mode) {
        std::print("строк {}, из них комментариев {}\n",
                   total_lines, comment_lines);
        std::print("событий всего : {}", parsed_events);
        for (const auto& [type, amount] : type_tally) {
            std::print("событий типа {} всего : {}", type, amount);
        }
    }

    return 0;
}