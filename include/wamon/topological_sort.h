#pragma once

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <queue>
#include <cassert>

namespace wamon {

template <typename NodeType>
class Graph {
 public:
  bool AddNode(NodeType node) {
    if (graph_.find(node) != graph_.end()) {
      return false;
    }
    graph_[node] = Edges{};
    return true;
  }

  bool AddEdge(NodeType from, NodeType to) {
    auto it = graph_.find(from);
    auto it_to = graph_.find(to);
    if (it == graph_.end() || it_to == graph_.end()) {
      return false;
    }
    if (it->second.FindEdge(to) == true) {
      return false;
    }
    bool succ = it->second.AddEdge(to);
    if (succ == true) {
      it_to->second.in_degree_ += 1;
    }
    return succ;
  }

  bool TopologicalSort() {
    std::queue<NodeType> queue;
    for(auto& each : graph_) {
      if (each.second.in_degree_ == 0) {
        queue.push(each.first);
      }
    }
    size_t out_count = 0;
    while(queue.empty() == false) {
      NodeType node = queue.front();
      queue.pop();
      for(auto& to : graph_[node].edges_) {
        assert(graph_[to].in_degree_ > 0);
        graph_[to].in_degree_ -= 1;
        if (graph_[to].in_degree_ == 0) {
          queue.push(to);
        }
      }
      out_count += 1;
    }
    return out_count == graph_.size();
  }

 private:
  struct Edges {
    std::vector<NodeType> edges_ = {};
    // 拓扑排序时in_degree_的值和edges_不保持一致。
    size_t in_degree_ = 0;

    bool FindEdge(const NodeType& to_node) {
      return std::find(edges_.begin(), edges_.end(), to_node) != edges_.end();
    }

    bool AddEdge(NodeType to) {
      if (FindEdge(to) == true) {
        return false;
      }
      edges_.push_back(to);
      return true;
    }
  };

  std::unordered_map<NodeType, Edges> graph_;
};

}