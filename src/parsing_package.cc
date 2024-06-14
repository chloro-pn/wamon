#include "wamon/parsing_package.h"

#include <algorithm>

#include "wamon/exception.h"
#include "wamon/package_unit.h"
#include "wamon/prepared_package_name.h"

namespace wamon {

void AssertInImportListOrThrow(PackageUnit& pu, const std::string& package_name) {
  if (package_name == pu.GetCurrentParsingPackage()) {
    return;
  }
  if (IsPreparedPakageName(package_name)) {
    return;
  }
  for (auto& each : pu.GetCurrentParsingImports()) {
    if (package_name == each) {
      return;
    }
  }
  throw WamonException("AssertInImportsOrThrow error, package {} not in packge {}'s import list", package_name,
                       pu.GetCurrentParsingPackage());
}

}  // namespace wamon
