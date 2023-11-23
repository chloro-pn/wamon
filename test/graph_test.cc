#include "gtest/gtest.h"
#include "wamon/topological_sort.h"

TEST(graph, basic) {
  wamon::Graph<int> graph;
  graph.AddNode(0);
  graph.AddNode(1);
  graph.AddNode(2);
  graph.AddNode(3);
  graph.AddEdge(0, 1);
  graph.AddEdge(1, 2);
  graph.AddEdge(2, 0);
  EXPECT_FALSE(graph.AddEdge(0, 1));
  EXPECT_FALSE(graph.AddNode(0));
  EXPECT_FALSE(graph.TopologicalSort());

  wamon::Graph<int> g2;
  g2.AddNode(0);
  g2.AddNode(1);
  g2.AddEdge(0, 1);
  EXPECT_TRUE(g2.TopologicalSort());
}