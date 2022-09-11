package(default_visibility = ["//visibility:public"])

cc_library(
  name = "wamon",
  hdrs = glob(["include/**/*.h"]),
  srcs = glob(["src/*.cc"]),
  includes = ["include"],
  deps = [
    "@fmt//:fmt",
  ]
)

cc_test(
  name = "wamon_test",
  srcs = glob(["test/*.cc"]),
  deps = [
    ":wamon",
    "@googletest//:gtest",
    "@googletest//:gtest_main",
  ],
)