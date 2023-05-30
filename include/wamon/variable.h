#pragma once

#include "wamon/type.h"

#include <string>

namespace wamon {

/*
 * Variable为运行时变量类型的基类，每个Variable一定包含一个type成员标识其类型
 * 以及一个变量名字，当其为空的时候标识匿名变量
 */
class Variable {
 public:
  explicit Variable(std::unique_ptr<Type>&& t, const std::string& name = "") : type_(std::move(t)), name_(name) {

  }

  virtual void ConstructByFields(std::vector<std::unique_ptr<Variable>>&& fields) = 0;

  virtual void DefaultConstruct() = 0;

  virtual std::unique_ptr<Variable> Clone() = 0;

 private:
  std::unique_ptr<Type> type_;
  std::string name_;
};

class StringVariable : public Variable {
 public:
  StringVariable(const std::string& v, const std::string& name) : Variable(TypeFactory<std::string>::Get(), name), value_(v) {

  }

 private:
  std::string value_;
};

}
