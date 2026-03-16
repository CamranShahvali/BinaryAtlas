#pragma once

#include <string>
#include <vector>

#include "binaryatlas/analysis/analysis_result.hpp"
#include "binaryatlas/core/analyzer.hpp"
#include "binaryatlas/core/binary_image.hpp"
#include "binaryatlas/disasm/instruction.hpp"
#include "binaryatlas/heuristics/finding.hpp"

namespace binaryatlas
{

class TerminalRenderer
{
public:
  [[nodiscard]] static std::string renderInspect(const BinaryImage& image, bool verbose);
  [[nodiscard]] static std::string renderDisassembly(const DisassemblyResult& disassembly,
                                                     bool verbose);
  [[nodiscard]] static std::string renderFunctions(const ProgramAnalysis& analysis, bool verbose);
  [[nodiscard]] static std::string renderCfg(const RecoveredFunction& function);
  [[nodiscard]] static std::string renderCallGraph(const ProgramAnalysis& analysis);
  [[nodiscard]] static std::string renderHeuristics(const std::vector<HeuristicFinding>& findings,
                                                    bool verbose);
  [[nodiscard]] static std::string renderAnalysis(const AnalysisBundle& bundle, bool verbose);
};

} // namespace binaryatlas
