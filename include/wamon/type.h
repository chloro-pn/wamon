#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "wamon/token.h"

/*
 * 类型系统：
 * - 基本类型：
 *   - 内置类型
 *     - int
 *     - string
 *     - byte
 *     - double
 *     - bool
 *     - void
 *   - 结构体类型
 * - 复合类型(类型加工器):
 *   - 指针类型 *       int*
 *   - 数组类型 [num]   int[2]
 *   - 函数类型 <-      int <- (type_list)
 * 
 */
namespace wamon {

class Type {
 public:
  virtual std::string GetTypeInfo() const = 0;
};

class BasicType : public Type {
 public:
  explicit BasicType(const std::string& type_name) : type_name_(type_name) {}

  std::string GetTypeInfo() const override { return type_name_; }

 private:
  std::string type_name_;
};

class CompoundType : public Type {};

class PointerType : public CompoundType {
 public:
 private:
  std::unique_ptr<Type> hold_type_;
};

class ArrayType : public CompoundType {
 public:
 private:
  size_t count_;
  std::unique_ptr<Type> hold_type_;
};

class FuncType : public CompoundType {
 public:
 private:
  std::unique_ptr<Type> return_type_;
  std::vector<std::unique_ptr<Type>> param_type_;
};

}  // namespace wamon