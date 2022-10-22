#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace wamon {

class StructDef {
 public:
  explicit StructDef(const std::string& name) : name_(name) {

  }

  const std::string& GetStructName() const {
    return name_;
  }

  void AddDataMember(const std::string& name, const std::string& type) {
    data_members_[name] = type;
  }
  
 private:
  std::string name_;
  std::unordered_map<std::string, std::string> data_members_;
};
}