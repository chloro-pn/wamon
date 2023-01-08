#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "wamon/type.h"

namespace wamon {

class StructDef {
 public:
  explicit StructDef(const std::string& name) : name_(name) {

  }

  const std::string& GetStructName() const {
    return name_;
  }

  void AddDataMember(const std::string& name, std::unique_ptr<Type>&& type) {
    data_members_[name] = std::move(type);
  }
  
 private:
  std::string name_;
  std::unordered_map<std::string, std::unique_ptr<Type>> data_members_;
};
}