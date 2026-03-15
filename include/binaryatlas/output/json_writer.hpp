#pragma once

#include <string>

#include "binaryatlas/analysis/analysis_result.hpp"
#include "binaryatlas/core/analyzer.hpp"
#include "binaryatlas/core/binary_image.hpp"
#include "binaryatlas/disasm/instruction.hpp"
#include "binaryatlas/heuristics/finding.hpp"

namespace binaryatlas
{

class JsonWriter
{
public:
  [[nodiscard]] static std::string renderInspect(const BinaryImage& image);
  [[nodiscard]] static std::string renderDisassembly(const DisassemblyResult& disassembly);
  [[nodiscard]] static std::string renderFunctions(const ProgramAnalysis& analysis);
  [[nodiscard]] static std::string renderHeuristics(
      const std::vector<HeuristicFinding>& findings);
  [[nodiscard]] static std::string renderAnalysis(const AnalysisBundle& bundle);
};

}  // namespace binaryatlas
