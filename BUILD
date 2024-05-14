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

cc_binary(
  name = "hello_world",
  srcs = ["example/hello_world.cc"],
  deps = [
    ":wamon"
  ],
)

cc_binary(
  name = "register_cpp_func",
  srcs = ["example/register_cpp_function.cc"],
  deps = [
    ":wamon"
  ],
)

cc_binary(
  name = "interpreter_api",
  srcs = ["example/interpreter_api.cc"],
  deps = [
    ":wamon"
  ],
)