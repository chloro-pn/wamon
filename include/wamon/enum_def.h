#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "wamon/exception.h"

namespace wamon {

class EnumDef {
 public:
  EnumDef(const std::string& enum_name) : enum_name_(enum_name) {}

  void AddEnumItem(const std::string& enum_id) {
    auto it = std::find(enum_items_.begin(), enum_items_.end(), enum_id);
    if (it != enum_items_.end()) {
      throw WamonException("EnumDef.AddEnumItem error, duplicate enum id {}", enum_id);
    }
    enum_items_.push_back(enum_id);
  }

  const std::string& GetEnumName() const { return enum_name_; }

  void SetEnumName(const std::string& name) { enum_name_ = name; }

  const std::vector<std::string>& GetEnumItems() const { return enum_items_; }

  bool ItemExist(const std::string& item) const {
    auto it = std::find(enum_items_.begin(), enum_items_.end(), item);
    return it != enum_items_.end();
  }

 private:
  std::string enum_name_;
  std::vector<std::string> enum_items_;
};

}  // namespace wamon
