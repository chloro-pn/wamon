#include "wamon/parser.h"
#include "wamon/scanner.h"

#include "gtest/gtest.h"

// just for test
#define private public
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
  )";

  auto tokens = scan.Scan(str);
  EXPECT_NO_THROW(wamon::Parse(tokens));
  EXPECT_EQ(wamon::PackageUnit::Instance().package_name_, "main");
  EXPECT_EQ(wamon::PackageUnit::Instance().import_packages_.size(), 2);
  EXPECT_EQ(wamon::PackageUnit::Instance().funcs_.size(), 1);
  EXPECT_EQ(wamon::PackageUnit::Instance().structs_.size(), 1);
  EXPECT_EQ(wamon::PackageUnit::Instance().var_define_.size(), 1);
  bool find = false;
  find = wamon::PackageUnit::Instance().funcs_.count("my_func") > 0;
  EXPECT_EQ(find, true);
  find = wamon::PackageUnit::Instance().structs_.count("my_struct_name") > 0;
  EXPECT_EQ(find, true);
  find = wamon::PackageUnit::Instance().var_define_[0]->GetType() == "bool";
  EXPECT_EQ(find, true);
}