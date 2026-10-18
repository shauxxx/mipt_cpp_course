#include "file_source.h"

#include <cstddef>
#include <cstdio>
#include <exception>
#include <print>
#include <stdexcept>

#include "event.h"
#include "parse.h"

namespace nano_edr {

FileSource::FileSource(const std::string& path) : file_(path) {
    if (!file_.is_open()) {
        throw std::runtime_error("не удалось открыть файл: " + path);
    }
}

void FileSource::Run(Agent* agent) {
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(file_, line)) {
        ++line_number;

        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            if (!IsBlankOrComment(line)) {
                std::print(stderr, "строка {} пропущена: не разобрана\n",
                           line_number);
            }
            continue;
        }

        try {
            const Event event(parts);
            agent->HandleEvent(event);
        } catch (const std::exception& e) {
            std::print(stderr, "строка {} пропущена: {}\n", line_number,
                       e.what());
        }
    }
}

}  // namespace nano_edr