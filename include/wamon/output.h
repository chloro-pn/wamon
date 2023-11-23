#pragma once

#include <iostream>
#include <string>
#include <string_view>

#include "fmt/format.h"

namespace wamon {

class Output {
 public:
  virtual void OutPutString(const std::string& v) = 0;
  virtual void OutPutInt(int v) = 0;
  virtual void OutPutDouble(double v) = 0;
  virtual void OutPutBool(bool v) = 0;
  virtual void OutPutByte(unsigned char c) = 0;

  // help method. format
  template <typename... Args>
  void OutputFormat(fmt::format_string<Args...> formator, Args&&... args) {
    OutPutString(fmt::format(formator, std::forward<Args>(args)...));
  }
};

void byte_to_string(unsigned char c, char (&buf)[4]);

class StdOutput : public Output {
 public:
  void OutPutString(const std::string& v) override { std::cout << v; }

  void OutPutInt(int v) override { std::cout << v; }

  void OutPutDouble(double v) override { std::cout << v; }

  void OutPutBool(bool v) override { std::cout << v; }

  void OutPutByte(unsigned char c) override {
    static thread_local char buf[4];
    byte_to_string(c, buf);
    std::cout << std::string_view(buf, 4);
  }
};

}  // namespace wamon
