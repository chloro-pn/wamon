#include "wamon/inner_type_method.h"

#include "gtest/gtest.h"

TEST(builtin_type_method, basic) {
  auto builtintype = wamon::TypeFactory<std::string>::Get();
  auto return_type = wamon::InnerTypeMethod::Instance().CheckAndGetReturnType(builtintype, "len", {});
  EXPECT_EQ(return_type->GetTypeInfo(), "int");

  builtintype = wamon::TypeFactory<std::vector<double>>::Get();
  std::vector<std::unique_ptr<wamon::Type>> params;
  params.push_back(wamon::TypeFactory<int>::Get());
  return_type = wamon::InnerTypeMethod::Instance().CheckAndGetReturnType(builtintype, "at", params);
  EXPECT_EQ(return_type->GetTypeInfo(), "double");
}