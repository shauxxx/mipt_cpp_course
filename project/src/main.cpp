#include <charconv>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <print>
#include <string>
#include <vector>

#include "../kit/include/l1.2/parse.h"
#include "../kit/include/l1.2/event_list.h"

int main(int argc, char** argv) {
    const std::vector<std::string> attributes = {
        "wscript.exe",
        ".locked",
        "certutil.exe",
        "\\Startup\\",
    };

    bool is_quiet  = false;
    long long window_size = 64;

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
            if (ec == std::errc() && n >= 0) { window_size = n; ++i; }
        }
    }

    nano_edr::EventList window{};
    window.head     = nullptr;
    window.tail     = nullptr;
    window.size     = 0;
    window.capacity = (std::size_t)window_size;

    long long lines = 0, comments = 0, events = 0;
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
            std::vector<const nano_edr::Event*> recent;
            for (nano_edr::EventNode* n = window.head; n; n = n->next)
                recent.push_back(&n->event);

            int total = (int)recent.size();
            int start = total > 2 ? total - 2 : 0;
            for (int k = start; k < total; ++k) {
                const nano_edr::Event* e = recent[k];
                std::print("[CTX] {}: ts={} type={} pid={}\n",
                           k - total, e->ts, e->type, e->pid);
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