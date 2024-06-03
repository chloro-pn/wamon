#include "wamon/parsing_package.h"

#include "wamon/exception.h"

namespace wamon {

std::string current_parsing_package;

std::vector<std::string> current_parsing_imports;

void AssertInImportListOrThrow(const std::string& package_name) {
  if (package_name == current_parsing_package) {
    return;
  }
  for (auto& each : current_parsing_imports) {
    if (package_name == each) {
      return;
    }
  }
  throw WamonException("AssertInImportsOrThrow error, package {} not in packge {}'s import list", package_name,
                       current_parsing_package);
}

}  // namespace wamon
