#include <catch2/catch_test_macros.hpp>

#include "binaryatlas/analysis/graph_algorithms.hpp"
#include "binaryatlas/analysis/metrics.hpp"

namespace
{

using namespace binaryatlas;

TEST_CASE("Tarjan SCC detection finds loop components")
{
  AdjacencyList adjacency {
      {1, {2}},
      {2, {3}},
      {3, {1, 4}},
      {4, {}},
  };

  const StronglyConnectedComponents scc = tarjanStronglyConnectedComponents(adjacency);
  REQUIRE(scc.components.size() == 2);
  CHECK(scc.components[0] == std::vector<Address>({1, 2, 3}));
  CHECK(scc.components[1] == std::vector<Address>({4}));
}

TEST_CASE("Function metrics compute cyclomatic complexity from CFG edges")
{
  RecoveredFunction function;
  function.entry = 0x1000;
  function.name = "synthetic";
  function.blocks = {
      {0x1000,
       0x1004,
       {0x1000},
       {{0x1000, 0x1010, BlockEdgeType::true_branch, true},
        {0x1000, 0x1020, BlockEdgeType::false_branch, true}},
       false},
      {0x1010, 0x1014, {0x1010}, {{0x1010, std::nullopt, BlockEdgeType::return_edge, false}}, false},
      {0x1020, 0x1024, {0x1020}, {{0x1020, std::nullopt, BlockEdgeType::return_edge, false}}, false},
  };

  const FunctionMetrics metrics = computeFunctionMetrics(function);
  CHECK(metrics.block_count == 3);
  CHECK(metrics.edge_count == 2);
  CHECK(metrics.cyclomatic_complexity == 2);
}

}  // namespace
