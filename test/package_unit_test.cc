#include "gtest/gtest.h"
// just for test
#define private public
#include "wamon/parser.h"
#include "wamon/scanner.h"

#include "wamon/package_unit.h"

TEST(package_unit, basic) {
  wamon::Scanner scan;

  std::string str = R"(
    package main;

    import net;
    import os;

    struct my_struct_name {
      int a;
      double b;
      string c;
    }

    func my_func(int age, string name) -> void {}

    let my_var : bool = (call init());

    method my_struct_name {
      func get_a() -> int {
        return self.a;
      }

      func get_b() -> double {
        return self.b;
      }
    }
  )";
  wamon::PackageUnit pu;
  auto tokens = scan.Scan(str);
  pu = wamon::Parse(tokens);
  EXPECT_EQ(pu.package_name_, "main");
  EXPECT_EQ(pu.import_packages_.size(), 2);
  EXPECT_EQ(pu.funcs_.size(), 1);
  EXPECT_EQ(pu.structs_.size(), 1);
  EXPECT_EQ(pu.var_define_.size(), 1);
  bool find = false;
  find = pu.funcs_.count("my_func") > 0;
  EXPECT_EQ(find, true);
  find = pu.structs_.count("my_struct_name") > 0;
  EXPECT_EQ(find, true);
  find = pu.var_define_[0]->GetType() == "bool";
  EXPECT_EQ(find, true);
}