#include "binaryatlas/core/analyzer.hpp"

#include "binaryatlas/format/binary_format.hpp"

namespace binaryatlas
{

Result<BinaryImage> BinaryAtlasEngine::load(const std::filesystem::path& path) const
{
  return BinaryLoader::load(path);
}

Result<DisassemblyResult> BinaryAtlasEngine::disassemble(
    const std::filesystem::path& path,
    const DisassemblyOptions& options) const
{
  Result<BinaryImage> image = load(path);
  if (!image)
  {
    return Result<DisassemblyResult>::failure(image.error());
  }

  Disassembler disassembler;
  return disassembler.disassemble(image.value(), options);
}

Result<AnalysisBundle> BinaryAtlasEngine::analyze(
    const std::filesystem::path& path,
    const AnalysisOptions& analysis_options,
    const HeuristicOptions& heuristic_options) const
{
  Result<BinaryImage> image = load(path);
  if (!image)
  {
    return Result<AnalysisBundle>::failure(image.error());
  }

  Disassembler disassembler;
  Result<DisassemblyResult> disassembly = disassembler.disassemble(image.value());
  if (!disassembly)
  {
    return Result<AnalysisBundle>::failure(disassembly.error());
  }

  ProgramAnalyzer analyzer;
  Result<ProgramAnalysis> analysis =
      analyzer.analyze(image.value(), disassembly.value(), analysis_options);
  if (!analysis)
  {
    return Result<AnalysisBundle>::failure(analysis.error());
  }

  HeuristicDetector detector;
  std::vector<HeuristicFinding> findings =
      detector.analyze(image.value(), disassembly.value(), analysis.value(), heuristic_options);
  AnalysisBundle bundle {
      std::move(image.value()),
      std::move(disassembly.value()),
      std::move(analysis.value()),
      std::move(findings)};
  return Result<AnalysisBundle>::success(std::move(bundle));
}

}  // namespace binaryatlas
