#include "wamon/parsing_package.h"

#include "wamon/exception.h"

namespace wamon {

std::string current_parsing_package;

std::vector<std::string> current_parsing_imports;

void AssertInImportListOrThrow(const std::string& package_name) {
  // wamon是特殊的包，注册的函数都会自动放在这个包里
  if (package_name == "wamon") {
    return;
  }
  for (auto& each : current_parsing_imports) {
    if (package_name == each) {
      return;
    }
  }
  throw WamonExecption("AssertInImportsOrThrow error, package {} not in packge {}'s import list", package_name,
                       current_parsing_package);
}

}  // namespace wamon
