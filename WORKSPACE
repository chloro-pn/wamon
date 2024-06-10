workspace(name = "com_github_wamon")

load('@bazel_tools//tools/build_defs/repo:git.bzl', 'git_repository')
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

https_proxy = "https://mirror.ghproxy.com/"

git_repository(
    name = "googletest",
    remote = https_proxy + "https://github.com/google/googletest",
    tag = "release-1.11.0",
)

git_repository(
    name = "catch2",
    remote = https_proxy + "https://github.com/catchorg/Catch2",
    tag = "v3.6.0",
)

git_repository(
    name = "fmt",
    tag = "8.1.1",
    remote = https_proxy + "https://github.com/fmtlib/fmt",
    patch_cmds = [
        "mv support/bazel/.bazelrc .bazelrc",
        "mv support/bazel/.bazelversion .bazelversion",
        "mv support/bazel/BUILD.bazel BUILD.bazel",
        "mv support/bazel/WORKSPACE.bazel WORKSPACE.bazel",
    ],
)

git_repository(
    name = "nlohmann_json",
    tag = "v3.11.3",
    remote = https_proxy + "https://github.com/nlohmann/json",
)

http_archive(
    name = "bazel_skylib",
    sha256 = "cd55a062e763b9349921f0f5db8c3933288dc8ba4f76dd9416aac68acee3cb94",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()
