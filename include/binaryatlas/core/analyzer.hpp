#pragma once

#include <filesystem>
#include <vector>

#include "binaryatlas/analysis/analysis_result.hpp"
#include "binaryatlas/analysis/recovery.hpp"
#include "binaryatlas/core/binary_image.hpp"
#include "binaryatlas/core/result.hpp"
#include "binaryatlas/disasm/disassembler.hpp"
#include "binaryatlas/heuristics/detector.hpp"

namespace binaryatlas
{

struct AnalysisBundle
{
  BinaryImage image;
  DisassemblyResult disassembly;
  ProgramAnalysis analysis;
  std::vector<HeuristicFinding> heuristic_findings;
};

class BinaryAtlasEngine
{
public:
  [[nodiscard]] Result<BinaryImage> load(const std::filesystem::path& path) const;
  [[nodiscard]] Result<DisassemblyResult> disassemble(
      const std::filesystem::path& path,
      const DisassemblyOptions& options = {}) const;
  [[nodiscard]] Result<AnalysisBundle> analyze(
      const std::filesystem::path& path,
      const AnalysisOptions& analysis_options = {},
      const HeuristicOptions& heuristic_options = {}) const;
};

}  // namespace binaryatlas
