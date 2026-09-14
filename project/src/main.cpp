// Каркас агента: читает журнал событий построчно и считает строки.
//
// Это заготовка занятия 1.1, а не решение. Детектов она не ищет — их вы
// добавите здесь же, в отмеченном месте ниже. Формат строки детекта, список
// признаков и правило про их порядок заданы в постановке занятия: по ним
// сравниваются эталоны.
//
// Весь код лежит в main, и на этом занятии так и надо: функции появятся
// на занятии 1.2, ссылки — на 1.3. Разбор аргументов, коды возврата и флаг
// --quiet — часть задания.
//
// Запуск:
//   nano-edr <журнал.log>
#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_map>

int main(int argc, char** argv) {
    // Аргументы разбираются грубо: путь к журналу и ничего больше. Остальное,
    // включая --quiet, добавляется по заданию.
    if (argc < 2) {
        std::print(stderr, "использование: nano-edr <журнал.log>\n");
        return 2;
    }

    std::ifstream log(argv[1]);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", argv[1]);
        return 2;
    }

    long long lines = 0;
    long long comments = 0;
    std::string line;

    std::vector<std::string> subs = {"wscript.exe", ".locked", "certutil.exe", "\\Startup\\"};
    std::vector<int> cnts = {0, 0, 0, 0};
    std::unordered_map<std::string, int> counter = {};

    while (std::getline(log, line)) {
        // Счётчик увеличивается до всех проверок: он считает строки файла,
        // а не события. Номер, посчитанный по событиям, бесполезен — по нему
        // нельзя открыть файл и посмотреть.
        ++lines;

        // Строки-комментарии в журнале начинаются с '#'. Они не события,
        // и детекта по ним быть не должно.
        if (!line.empty() && line[0] == '#') {
            ++comments;
            continue;
        }

        // >>> Здесь начинается занятие 1.1.
        for (int i = 0; i < 4; i++) {
            if(line.find(subs[i]) != std::string::npos && line.size() > 0 && line[0] != '#') {
                std::cout << "[DETECT] строка " << lines << ", признак " << subs[i] << ": " << line << std::endl;
                cnts[i]++;

            }
        
        }
        std::string ername = line.substr(line.find("type") + 5, line.find(" ", line.find("type") + 5) - (line.find("type") + 5));
        counter[ername]++;
        // Проверка признаков и печать детекта. Номер строки, который нужен
        // в выводе, — это lines.
    }
    bool flag = true;
    for(int i = 0; i < argc; ++i) {
        if(std::string(argv[i]) == "--quiet") {
            flag = false;
        }
    }

    if(flag) {
        for (auto [key, val]:counter) {
            std::print("{}: {} \n", key, val);
        }
        std::print("строк {}, из них комментариев {}\n", lines, comments);
    }

    return 0;
}