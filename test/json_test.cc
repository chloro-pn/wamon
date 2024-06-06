#include "nlohmann/json.hpp"

#include "gtest/gtest.h"

TEST(json_test, basic) {
  nlohmann::json j = nlohmann::json::parse(R"("hello world")");
  std::string s = j.dump();
  EXPECT_EQ(s, "\"hello world\"");
}
