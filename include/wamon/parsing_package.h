#pragma once

#include <string>
#include <vector>

namespace wamon {

extern std::string current_parsing_package;

extern std::vector<std::string> current_parsing_imports;

void AssertInImportListOrThrow(const std::string& package_name);

}  // namespace wamon
