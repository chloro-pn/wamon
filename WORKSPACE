load('@bazel_tools//tools/build_defs/repo:git.bzl', 'git_repository')

git_repository(
    name = "googletest",
    remote = "https://mirror.ghproxy.com/https://github.com/google/googletest",
    tag = "release-1.11.0",
)

git_repository(
    name = "fmt",
    tag = "8.1.1",
    remote = "https://mirror.ghproxy.com/https://github.com/fmtlib/fmt",
    patch_cmds = [
        "mv support/bazel/.bazelrc .bazelrc",
        "mv support/bazel/.bazelversion .bazelversion",
        "mv support/bazel/BUILD.bazel BUILD.bazel",
        "mv support/bazel/WORKSPACE.bazel WORKSPACE.bazel",
    ],
)