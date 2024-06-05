#pragma once

#include <string>

namespace wamon {

class PackageUnit;
void AssertInImportListOrThrow(PackageUnit& pu, const std::string& package_name);

}  // namespace wamon
