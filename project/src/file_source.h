#ifndef NANO_EDR_FILE_SOURCE_H
#define NANO_EDR_FILE_SOURCE_H

#include <fstream>
#include <string>

#include "agent.h"

namespace nano_edr {

class FileSource {
 public:
    explicit FileSource(const std::string& path);

    // Читает файл до конца: строка -> EventParts -> Event -> HandleEvent.
    // Испорченную строку или событие пропускает и сообщает в stderr.
    void Run(Agent* agent);

 private:
    std::ifstream file_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_FILE_SOURCE_H