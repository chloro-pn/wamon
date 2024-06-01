#include <iostream>
#include <string>

#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type_checker.h"
#include "wamon/variable.h"

int main() {
  std::string script = R"(
    package sort;

    let datas : list(int) = (5, 1, 3, 2, 4, 8, 6, 7);

    func merge_sort(list(int) ref datas, int begin, int end) -> list(int) {
      if (end < begin) {
        return new list(int)();
      }
      if (begin == end) {
        return new list(int)(datas[begin]);
      }
      let ret : list(int) = ();
  
      int mid = (begin + end) / 2;
      let left_list : list(int) = merge_sort(datas, begin, mid);
      let right_list : list(int) = merge_sort(datas, mid + 1, end);
      let left_index : int = 0;
      let right_index : int = 0;

      let index : int = 0;
      for(index = begin; index <= end; index = index + 1) {
        if (left_index == call left_list:size()) {
          call ret:push_back(right_list[right_index]);
          right_index = right_index + 1;
        } else {
          if (right_index == call right_list:size()) {
            call ret:push_back(left_list[left_index]);
            left_index = left_index + 1;
          } else {
            if (left_list[left_index] < right_list[right_index]) {
              call ret:push_back(left_list[left_index]);
              left_index = left_index + 1;
            } else {
              call ret:push_back(right_list[right_index]);
              right_index = right_index + 1;
            }
          }
        }
      }
      return move ret;
    }
  )";

  wamon::Scanner scanner;
  auto tokens = scanner.Scan(script);
  wamon::PackageUnit package_unit = wamon::Parse(tokens);
  package_unit = wamon::MergePackageUnits(std::move(package_unit));
  wamon::TypeChecker type_checker(package_unit);

  std::string reason;
  bool succ = type_checker.CheckAll(reason);
  if (succ == false) {
    std::cerr << "type check error : " << reason << std::endl;
    return -1;
  }
  wamon::Interpreter ip(package_unit);
  auto ret = ip.CallFunctionByName("sort$merge_sort", {});
  wamon::StdOutput out;
  ret->Print(out);
  return 0;
}
