#pragma once

#include <unordered_map>
#include <vector>
#include <algorithm>

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
    if (it == graph_.end()) {
      return false;
    }
    if (it->second.FindEdge(to) == true) {
      return false;
    }
    return it->second.AddEdge(to);
  }

  bool TopologicalSort() {

  }

 private:
  struct Edges {
    std::vector<NodeType> edges_;

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