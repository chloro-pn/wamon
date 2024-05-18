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

  t3 = wamon::TypeFactory<WAMON_STRUCT_TYPE("msn")>::Get();
  wamon::SetScopeForStructType(t3, "main");
  EXPECT_EQ(t3->IsBasicType(), true);
  EXPECT_EQ(t3->GetTypeInfo(), "main$msn");

  auto t4 = wamon::TypeFactory<void(int, WAMON_STRUCT_TYPE("msn"))>::Get();
  EXPECT_TRUE(wamon::IsFuncType(t4));
  EXPECT_EQ(*wamon::GetReturnType(t4), *wamon::TypeFactory<void>::Get());
  EXPECT_NE(*wamon::GetParamType(t4)[1], *t3);
}

TEST(type, get_struct_name) {
  auto struct_name = wamon::CreateStructName<"my_struct_name">();
  EXPECT_EQ(std::string(struct_name.data), std::string("my_struct_name"));
  EXPECT_EQ(struct_name.n, std::string("my_struct_name").size() + 1);
}
