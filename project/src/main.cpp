#include <charconv>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <print>
#include <string>
#include <vector>

#include "parse.h"
#include "event_list.h"

int main(int argc, char** argv) {
    bool is_quiet  = false;
    long long window_size = 64;
    const std::vector<std::string> attributes = {
        "wscript.exe",
        ".locked",
        "certutil.exe",
        "\\Startup\\",
    };

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
        std::string a = argv[i];
        if (a == "--quiet") {
            is_quiet = true;
        } else if (a == "--window-size" && i + 1 < argc) {
            const char* s = argv[i + 1];
            long long n = 0;
            auto [p, ec] = std::from_chars(s, s + std::strlen(s), n);
            if (ec == std::errc() && n >= 0) {
                window_size = n; ++i; 
            }
        }
    }

    nano_edr::EventList window{};
    window.head = nullptr;
    window.tail = nullptr;
    window.size = 0;
    window.capacity = static_cast<std::size_t>(window_size);

    long long lines = 0;
    long long comments = 0;
    long long events = 0;
    std::map<std::string, long long> event_types_count;
    std::string line;

    while (std::getline(log, line)) {
        ++lines;
        if (nano_edr::IsBlankOrComment(&line)) {
            if (!line.empty()) ++comments;
            continue;
        }

        bool detected = false;
        for (auto& attribute : attributes) {
            if (line.find(attribute) != std::string::npos) {
                std::print("[DETECT] строка {}, признак {}: {}\n",
                           lines, attribute, line);
                detected = true;
            }
        }

        nano_edr::Event ev;
        if (!nano_edr::ParseEventLine(&line, &ev)) continue;

        ++events;
        ++event_types_count[ev.type];

        if (detected && !is_quiet) {

            size_t total = window.size;

            if (total == 0) {
                continue;
            }
            if (total == 1) {
                std::print("[CTX] -1: ts={} type={} pid={}\n", window.tail->event.ts, window.tail->event.type, window.tail->event.pid);
            }
            else {
                nano_edr::EventNode* cur = window.head;
                for (size_t k = 0; k < total; k++) {
                    if (k >= total - 2) {
                        std::print("[CTX] -{}: ts={} type={} pid={}\n", total - k, cur->event.ts, cur->event.type, cur->event.pid);
                    }
                    cur = cur->next;
                    
                }
            }


        }

        ListPushBack(&window, &ev);
    }

    if (!is_quiet) {
        std::print("строк {}, из них комментариев {}\n", lines, comments);
        std::print("событий всего : {}\n", events);
        for (auto& [type, count] : event_types_count)
            std::print("событий типа {} всего : {}\n", type, count);
    }
    return 0;
}