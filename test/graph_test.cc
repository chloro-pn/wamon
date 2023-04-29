#include "wamon/topological_sort.h"

#include "gtest/gtest.h"

TEST(graph, basic) {
  wamon::Graph<int> graph;
  graph.AddNode(0);
  graph.AddNode(1);
  graph.AddNode(2);
  graph.AddEdge(0, 1);
  graph.AddEdge(0, 2);
  EXPECT_FALSE(graph.AddEdge(0, 2));
  EXPECT_FALSE(graph.AddNode(0));
}