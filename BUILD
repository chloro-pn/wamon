package(default_visibility = ["//visibility:public"])

load("//bazel/config:copt.bzl", "WAMON_COPTS")

cc_library(
  name = "wamon",
  hdrs = glob(["include/**/*.h"]),
  srcs = glob(["src/*.cc"]),
  includes = ["include"],
  copts = WAMON_COPTS,
  deps = [
    "@fmt//:fmt",
  ]
)

cc_test(
  name = "wamon_test",
  srcs = glob(["test/*.cc"]),
  copts = WAMON_COPTS,
  deps = [
    ":wamon",
    "@googletest//:gtest",
    "@googletest//:gtest_main",
  ],
)

cc_binary(
  name = "hello_world",
  srcs = ["example/hello_world.cc"],
  copts = WAMON_COPTS,
  deps = [
    ":wamon"
  ],
)

cc_binary(
  name = "register_cpp_func",
  srcs = ["example/register_cpp_function.cc"],
  copts = WAMON_COPTS,
  deps = [
    ":wamon"
  ],
)

cc_binary(
  name = "interpreter_api",
  srcs = ["example/interpreter_api.cc"],
  copts = WAMON_COPTS,
  deps = [
    ":wamon"
  ],
)

cc_binary(
  name = "lambda",
  srcs = ["example/lambda.cc"],
  copts = WAMON_COPTS,
  deps = [
    ":wamon"
  ],
)

cc_binary(
  name = "struct_trait",
  srcs = ["example/struct_trait.cc"],
  copts = WAMON_COPTS,
  deps = [
    ":wamon"
  ],
)

cc_binary(
  name = "merge_sort",
  srcs = ["example/merge_sort.cc"],
  copts = WAMON_COPTS,
  deps = [
    ":wamon"
  ],
)