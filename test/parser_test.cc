#include "wamon/parser.h"

#include "gtest/gtest.h"
#include "wamon/scanner.h"

TEST(parser, basic) {
  wamon::Scanner scan;
  std::string str = "struct test [int a;]";
  auto tokens = scan.Scan(str);
  EXPECT_THROW(wamon::Parse(tokens), std::runtime_error);

  str = R"(
    struct my_struct_name {
      int a;
      double b;
      string c;
    }
  )";

  tokens = scan.Scan(str);
  EXPECT_NO_THROW(wamon::Parse(tokens));
}