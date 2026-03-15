#include "binaryatlas/analysis/metrics.hpp"

#include <algorithm>
#include <set>
#include <unordered_map>

#include "binaryatlas/analysis/graph_algorithms.hpp"

namespace binaryatlas
{
namespace
{

[[nodiscard]] AdjacencyList buildAdjacency(const RecoveredFunction& function)
{
  AdjacencyList adjacency;
  for (const BasicBlock& block : function.blocks)
  {
    adjacency[block.start];
    for (const BlockEdge& edge : block.outgoing_edges)
    {
      if (edge.target.has_value() && edge.intra_function)
      {
        adjacency[block.start].push_back(*edge.target);
      }
    }
    std::sort(adjacency[block.start].begin(), adjacency[block.start].end());
    adjacency[block.start].erase(
        std::unique(adjacency[block.start].begin(), adjacency[block.start].end()),
        adjacency[block.start].end());
  }
  return adjacency;
}

}  // namespace

FunctionMetrics computeFunctionMetrics(const RecoveredFunction& function)
{
  FunctionMetrics metrics;
  metrics.block_count = function.blocks.size();
  metrics.call_out_degree = function.callees.size();

  std::unordered_map<Address, std::size_t> indegree;
  std::size_t internal_edge_count = 0;
  std::size_t exit_edge_count = 0;
  std::uint64_t size_bytes = 0;

  AdjacencyList adjacency = buildAdjacency(function);
  for (const BasicBlock& block : function.blocks)
  {
    metrics.instruction_count += block.instruction_addresses.size();
    if (block.end >= block.start)
    {
      size_bytes += block.end - block.start;
    }

    std::size_t out_degree = 0;
    for (const BlockEdge& edge : block.outgoing_edges)
    {
      if (edge.target.has_value() && edge.intra_function)
      {
        ++internal_edge_count;
        ++out_degree;
        ++indegree[*edge.target];
      }
      else if (edge.type != BlockEdgeType::call_fallthrough)
      {
        ++exit_edge_count;
      }
    }
    metrics.max_out_degree = std::max(metrics.max_out_degree, out_degree);
  }

  metrics.size_bytes = size_bytes;
  metrics.edge_count = internal_edge_count;
  metrics.average_out_degree =
      metrics.block_count == 0
          ? 0.0
          : static_cast<double>(internal_edge_count) /
                static_cast<double>(metrics.block_count);

  for (const BasicBlock& block : function.blocks)
  {
    metrics.max_in_degree = std::max(metrics.max_in_degree, indegree[block.start]);
  }

  StronglyConnectedComponents scc = tarjanStronglyConnectedComponents(adjacency);
  for (const std::vector<Address>& component : scc.components)
  {
    if (component.size() > 1)
    {
      ++metrics.loop_count;
      continue;
    }

    const auto iterator = adjacency.find(component.front());
    if (iterator != adjacency.end() &&
        std::find(iterator->second.begin(), iterator->second.end(), component.front()) !=
            iterator->second.end())
    {
      ++metrics.loop_count;
    }
  }

  metrics.connected_components = adjacency.empty() ? 0 : connectedComponentCount(adjacency);

  const std::size_t complexity_edges = internal_edge_count + exit_edge_count;
  const std::size_t complexity_nodes = metrics.block_count + (exit_edge_count > 0 ? 1U : 0U);
  const std::size_t complexity_components =
      metrics.connected_components == 0 ? 1U : metrics.connected_components;
  const std::int64_t complexity = static_cast<std::int64_t>(complexity_edges) -
                                  static_cast<std::int64_t>(complexity_nodes) +
                                  static_cast<std::int64_t>(2 * complexity_components);
  metrics.cyclomatic_complexity = complexity <= 0 ? 1U : static_cast<std::size_t>(complexity);

  return metrics;
}

ProgramMetrics computeProgramMetrics(
    const std::vector<RecoveredFunction>& functions,
    const std::vector<CallGraphEdge>& call_graph)
{
  ProgramMetrics metrics;
  metrics.function_count = functions.size();
  metrics.call_edge_count = call_graph.size();

  for (const RecoveredFunction& function : functions)
  {
    metrics.block_count += function.metrics.block_count;
    metrics.instruction_count += function.metrics.instruction_count;
    if (function.metrics.loop_count > 0)
    {
      ++metrics.looping_function_count;
    }
    if (function.metrics.max_out_degree >= 5 || function.metrics.call_out_degree >= 5)
    {
      ++metrics.dispatcher_candidate_count;
    }
  }

  return metrics;
}

}  // namespace binaryatlas
