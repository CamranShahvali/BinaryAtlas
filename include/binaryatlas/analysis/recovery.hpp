#pragma once

#include <cstddef>

#include "binaryatlas/analysis/analysis_result.hpp"
#include "binaryatlas/core/binary_image.hpp"
#include "binaryatlas/core/result.hpp"
#include "binaryatlas/disasm/instruction.hpp"

namespace binaryatlas
{

struct AnalysisOptions
{
  bool enable_prologue_heuristics {true};
  std::size_t high_fanout_threshold {5};
  std::size_t complexity_threshold {8};
};

class ProgramAnalyzer
{
public:
  [[nodiscard]] Result<ProgramAnalysis> analyze(
      const BinaryImage& image,
      const DisassemblyResult& disassembly,
      const AnalysisOptions& options = {}) const;
};

}  // namespace binaryatlas
