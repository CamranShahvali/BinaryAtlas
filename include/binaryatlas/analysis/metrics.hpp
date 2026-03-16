#pragma once

#include "binaryatlas/analysis/function.hpp"

namespace binaryatlas
{

struct ProgramMetrics
{
  std::size_t function_count{};
  std::size_t block_count{};
  std::size_t instruction_count{};
  std::size_t call_edge_count{};
  std::size_t looping_function_count{};
  std::size_t dispatcher_candidate_count{};
};

[[nodiscard]] FunctionMetrics computeFunctionMetrics(const RecoveredFunction& function);
[[nodiscard]] ProgramMetrics computeProgramMetrics(const std::vector<RecoveredFunction>& functions,
                                                   const std::vector<CallGraphEdge>& call_graph);

} // namespace binaryatlas
