#ifndef NANO_EDR_KIT_TEXT_H
#define NANO_EDR_KIT_TEXT_H

#include <string>

#include "event.h"
#include "conditions.h"

namespace nano_edr {

bool EqualLowerCase(const std::string& value1, const std::string& value2);

bool ContainsLowerCase(const std::string& value1, const std::string& value2);

bool EndIsLowerCase(const std::string& value1, const std::string& value2);

} 

#endif