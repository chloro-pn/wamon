workspace(name = "com_github_wamon")

load('@bazel_tools//tools/build_defs/repo:git.bzl', 'git_repository')

https_proxy = "https://mirror.ghproxy.com/"

git_repository(
    name = "googletest",
    remote = https_proxy + "https://github.com/google/googletest",
    tag = "release-1.11.0",
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
