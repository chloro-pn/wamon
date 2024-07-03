#pragma once

#include "wamon/variable.h"

namespace wamon {

inline void byte_to_string(unsigned char c, char (&buf)[4]) {
  buf[0] = '0';
  buf[1] = 'X';
  int h = static_cast<int>(c) / 16;
  int l = static_cast<int>(c) - h * 16;
  if (h >= 0 && h <= 9) {
    buf[2] = h + '0';
  } else {
    buf[2] = h - 10 + 'A';
  }
  if (l >= 0 && l <= 9) {
    buf[3] = l + '0';
  } else {
    buf[3] = l - 10 + 'A';
  }
}

class ByteVariable : public Variable {
 public:
  ByteVariable(unsigned char v, ValueCategory vc, const std::string& name)
      : Variable(TypeFactory<unsigned char>::Get(), vc, name), value_(v) {}

  unsigned char GetValue() const { return value_; }

  void SetValue(unsigned char c) { value_ = c; }

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override {
    if (fields.size() != 1) {
      throw WamonException("ByteVariable's ConstructByFields method error : fields.size() == {}", fields.size());
    }
    if (fields[0]->GetTypeInfo() != GetTypeInfo()) {
      throw WamonException("ByteVariable's ConstructByFields method error, type dismatch : {} != {}",
                           fields[0]->GetTypeInfo(), GetTypeInfo());
    }
    ByteVariable* ptr = static_cast<ByteVariable*>(fields[0].get());
    value_ = ptr->GetValue();
  }

  void DefaultConstruct() override { value_ = 0; }

  std::shared_ptr<Variable> Clone() override {
    return std::make_shared<ByteVariable>(GetValue(), ValueCategory::RValue, "");
  }

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    return value_ == static_cast<ByteVariable*>(other.get())->value_;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    value_ = static_cast<ByteVariable*>(other.get())->value_;
  }

  nlohmann::json Print() override {
    char buf[4];
    byte_to_string(value_, buf);
    return nlohmann::json(std::string(buf, 4));
  }

 private:
  unsigned char value_;
};

inline ByteVariable* AsByteVariable(const std::shared_ptr<Variable>& v) { return static_cast<ByteVariable*>(v.get()); }

}  // namespace wamon
