#include "wamon/parsing_package.h"

#include "wamon/exception.h"

namespace wamon {

std::string current_parsing_package;

std::vector<std::string> current_parsing_imports;

void AssertInImportListOrThrow(const std::string& package_name) {
  for (auto& each : current_parsing_imports) {
    if (package_name == each) {
      return;
    }
  }
  throw WamonExecption("AssertInImportsOrThrow error, package {} not in packge {}'s import list", package_name,
                       current_parsing_package);
}

}  // namespace wamon
