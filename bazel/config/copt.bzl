WAMON_COPTS = select({
  "@com_github_wamon//bazel/config:msvc_compiler": [
    "/std:c++20",
    "/EHa"
  ],
  "//conditions:default": [
    "-std=c++20",
    "-D_GLIBCXX_USE_CXX11_ABI=1",
    "-Wall",
    "-Werror",
    "-g",
  ],
}

)