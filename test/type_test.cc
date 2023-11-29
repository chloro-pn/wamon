#include "wamon/type.h"

#include "gtest/gtest.h"

TEST(type, factory) {
  auto t1 = wamon::TypeFactory<void>::Get();
  auto t2 = wamon::TypeFactory<unsigned char>::Get();
  EXPECT_EQ(*t1, *std::make_unique<wamon::BasicType>("void"));
  EXPECT_EQ(*t2, *std::make_unique<wamon::BasicType>("byte"));

  auto t3 = wamon::TypeFactory<void(int, double)>::Get();
  std::vector<std::unique_ptr<wamon::Type>> params_type;
  params_type.push_back(std::make_unique<wamon::BasicType>("int"));
  params_type.push_back(std::make_unique<wamon::BasicType>("double"));
  auto tmp = std::make_unique<wamon::FuncType>(std::move(params_type), std::move(t1));
  EXPECT_EQ(*t3, *tmp);

  t3 = wamon::TypeFactory<void()>::Get();
  EXPECT_TRUE(wamon::IsFuncType(t3));
  params_type.clear();
  tmp = std::make_unique<wamon::FuncType>(std::move(params_type), wamon::TypeFactory<void>::Get());
  EXPECT_EQ(*t3, *tmp);
}