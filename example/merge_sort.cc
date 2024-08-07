#include <iostream>
#include <string>

#include "wamon/interpreter.h"
#include "wamon/parser.h"
#include "wamon/scanner.h"
#include "wamon/type_checker.h"
#include "wamon/variable.h"
#include "wamon/variable_list.h"

int main() {
  std::string script = R"(
    package sort;

    let datas : list(int) = (5, 1, 3, 2, 4, 8, 6, 7);

    func MergeSort() -> list(int) {
      return call merge_sort:(datas, 0, call datas:size() - 1);
    }

    func merge_sort(list(int) ref datas, int begin, int end) -> list(int) {
      if (end < begin) {
        return new list(int)();
      }
      if (begin == end) {
        return new list(int)(datas[begin]);
      }
      let ret : list(int) = ();
  
      let mid : int = ((begin + end) / 2);
      let ll : list(int) = call merge_sort:(datas, begin, mid);
      let rl : list(int) = call merge_sort:(datas, mid + 1, end);
      let li : int = 0;
      let ri : int = 0;

      let i : int = 0;
      for(i = begin; i <= end; ++i) {
        if (li == call ll:size()) {
          call ret:push_back(rl[ri]);
          ++ri;
        } elif (ri == call rl:size()) {
          call ret:push_back(ll[li]);
          ++li;
        } elif (ll[li] < rl[ri]) {
          call ret:push_back(ll[li]);
          ++li;
        } else {
          call ret:push_back(rl[ri]);
          ++ri;
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
  auto ret = ip.CallFunctionByName("sort$MergeSort", {});
  auto v = wamon::AsListVariable(ret);
  std::cout << "after merge sort : ";
  for (size_t i = 0; i < v->Size(); ++i) {
    std::cout << wamon::AsIntVariable(v->at(i))->GetValue() << " ";
  }
  std::cout << std::endl;
  return 0;
}
