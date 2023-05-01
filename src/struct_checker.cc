#include "wamon/struct_checker.h"
#include "wamon/static_analyzer.h"
#include "wamon/package_unit.h"
#include "wamon/exception.h"

namespace wamon {

void StructChecker::CheckStructs() {
  const PackageUnit& pu = static_analyzer_.GetPackageUnit();
  const auto& structs = pu.GetStructs();
  auto built_in_types = GetBuiltInTypesWithoutVoid();
  // 保证不同类型的TypeInfo不相同，因此可以用其代表一个类型进行循环依赖分析
  bool all_succ = true;
  Graph<std::string> graph;
  for(const auto& each : built_in_types) {
    all_succ &= graph.AddNode(each);
  }
  for(const auto& each : structs) {
    all_succ &= graph.AddNode(each.first);
  }
  // 如果类型a依赖于类型b，插入一条a->b有向边
  for(const auto& each : structs) {
    auto dependent = each.second->GetDependent();
    for(auto& to : dependent) {
      graph.AddEdge(each.first, to);
    }
  }
  if (graph.TopologicalSort() == false) {
    throw WamonExecption("struct dependent check error");
  }
}

}