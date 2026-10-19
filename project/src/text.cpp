#include "text.h"

#include <string>

namespace nano_edr {

namespace {
char ToLower(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}
}

bool EqualLowerCase(const std::string& value1, const std::string& value2) { 
    if (value1.size() != value2.size()) {
        return false;
    }
    for (std::size_t i = 0; i < value1.size(); ++i) {
        if (ToLower(value1[i]) != ToLower(value2[i])) {
            return false;
        }
    }
    return true;
}

bool ContainsLowerCase(const std::string& value1, const std::string& value2) {
    if (value2.empty()) {
        return true;
    }
    if (value1.size() < value2.size()) {
        return false;
    }
    for (std::size_t i = 0; i + value2.size() <= value1.size(); ++i) {
        if (ToLower(value1[i]) == ToLower(value2[0])) {
            if (EqualLowerCase(value1.substr(i, value2.size()), value2)) {
                return true;
            }
        }
    }
    return false;
}

bool EndIsLowerCase(const std::string& value1, const std::string& value2) {
    if (value1.size() < value2.size()) {
        return false;
    }
    return EqualLowerCase(value1.substr(value1.size() - value2.size(), value2.size()), value2);
}

}