#include <string>

#include "wamon/type.h"

namespace wamon {

/*
 * 运行时变量，每个变量绑定一个类型
 */
class Variable {
 public:
  Variable(std::unique_ptr<Type>&& type, const std::string& name) : type_(std::move(type)), var_name_(name) {

  }

  const Type* GetType() const {
    return type_.get();
  }

  const std::string& GetName() const {
    return var_name_;
  }

 protected:
  std::unique_ptr<Type> type_;
  // 变量名字，可以为空，为空则是匿名变量
  std::string var_name_;
};

class StringVariable : public Variable {
 public:
  explicit StringVariable(const std::string& name) : Variable(std::make_unique<BasicType>("string"), name) {

  }

  void SetValue(const std::string& v) {
    value_ = v;
  }

  const std::string& GetValue() const {
    return value_;
  }

 private:
  std::string value_;
};

}