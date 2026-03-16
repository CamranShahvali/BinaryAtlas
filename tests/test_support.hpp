#pragma once

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string_view>

#include "binaryatlas/core/analyzer.hpp"
#include "binaryatlas/format/binary_format.hpp"

namespace binaryatlas::test_support
{

inline std::filesystem::path fixturePath(std::string_view name)
{
  return std::filesystem::path(BINARYATLAS_TEST_FIXTURE_DIR) / std::string(name);
}

inline Result<BinaryImage> loadFixture(std::string_view name)
{
  return BinaryLoader::load(fixturePath(name));
}

inline Result<AnalysisBundle> analyzeFixture(std::string_view name)
{
  BinaryAtlasEngine engine;
  return engine.analyze(fixturePath(name));
}

inline const RecoveredFunction* findFunctionByName(const ProgramAnalysis& analysis,
                                                   std::string_view name)
{
  const auto iterator =
      std::find_if(analysis.functions.begin(), analysis.functions.end(),
                   [name](const RecoveredFunction& function) { return function.name == name; });
  return iterator == analysis.functions.end() ? nullptr : &*iterator;
}

inline bool hasCallEdge(const ProgramAnalysis& analysis, Address source, Address target)
{
  return std::any_of(analysis.call_graph.begin(), analysis.call_graph.end(),
                     [source, target](const CallGraphEdge& edge)
                     { return edge.source == source && edge.target == target; });
}

inline bool hasFinding(const std::vector<HeuristicFinding>& findings, std::string_view category)
{
  return std::any_of(findings.begin(), findings.end(), [category](const HeuristicFinding& finding)
                     { return finding.category == category; });
}

} // namespace binaryatlas::test_support
