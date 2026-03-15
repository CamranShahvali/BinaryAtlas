#pragma once

#include <string>

#include "binaryatlas/analysis/analysis_result.hpp"
#include "binaryatlas/analysis/function.hpp"

namespace binaryatlas
{

class DotExporter
{
public:
  [[nodiscard]] static std::string exportFunctionCfg(const RecoveredFunction& function);
  [[nodiscard]] static std::string exportCallGraph(const ProgramAnalysis& analysis);
};

}  // namespace binaryatlas
