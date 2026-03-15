#pragma once

#include <vector>

#include "binaryatlas/analysis/function.hpp"
#include "binaryatlas/analysis/metrics.hpp"

namespace binaryatlas
{

struct ProgramAnalysis
{
  std::vector<RecoveredFunction> functions;
  std::vector<CallGraphEdge> call_graph;
  ProgramMetrics metrics;
  std::vector<FunctionSeed> unused_seeds;
};

}  // namespace binaryatlas
