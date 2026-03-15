#include <cstring>

#include <catch2/catch_test_macros.hpp>

#include "binaryatlas/heuristics/detector.hpp"

namespace
{

using namespace binaryatlas;

TEST_CASE("Heuristic detector scores virtual dispatch, jump tables, engine strings, and vtable regions")
{
  BinaryMetadata metadata;
  metadata.format = BinaryFormat::elf;
  metadata.architecture = Architecture::x86_64;
  metadata.endianness = Endianness::little;
  metadata.entry_point = 0x1000;
  metadata.source_path = "synthetic";

  std::vector<std::uint8_t> bytes(0x40, 0);
  const std::uint64_t pointers[] = {0x1000, 0x1008, 0x1010};
  std::memcpy(bytes.data() + 0x20, pointers, sizeof(pointers));

  std::vector<Section> sections {
      {".text", {0x1000, 0x1020}, 0x00, 0x20, 16, 0, 0, true, false, true},
      {".rodata", {0x2000, 0x2020}, 0x20, 0x20, 8, 0, 0, false, false, true},
  };

  BinaryImage image(
      metadata,
      "synthetic",
      std::move(bytes),
      sections,
      {},
      {},
      {},
      {},
      {},
      {{"rodata", 0x2008, "DirectX Present hook candidate"},
       {"rodata", 0x2018, "UnityPlayer subsystem"},
       {"rodata", 0x2028, "OpenXR runtime integration"}});

  RecoveredFunction function;
  function.entry = 0x1000;
  function.name = "render_present_hook";
  function.call_sites = {{0x1004, std::nullopt, true, false, "qword ptr [rax + 8]"}};
  function.referenced_strings = {
      "DirectX Present hook candidate",
      "UnityPlayer subsystem",
      "OpenXR runtime integration"};
  function.metrics.block_count = 6;
  function.metrics.max_out_degree = 6;
  function.metrics.call_out_degree = 0;
  function.metrics.cyclomatic_complexity = 9;
  function.blocks = {
      {0x1000, 0x1008, {0x1000}, {{0x1000, std::nullopt, BlockEdgeType::unresolved_indirect, false}}, false},
  };

  ProgramAnalysis analysis;
  analysis.functions = {function};

  HeuristicDetector detector;
  const std::vector<HeuristicFinding> findings = detector.analyze(image, {}, analysis);

  CHECK(std::any_of(findings.begin(), findings.end(), [](const HeuristicFinding& finding) {
    return finding.category == "candidate_virtual_dispatch";
  }));
  CHECK(std::any_of(findings.begin(), findings.end(), [](const HeuristicFinding& finding) {
    return finding.category == "candidate_jump_table";
  }));
  CHECK(std::any_of(findings.begin(), findings.end(), [](const HeuristicFinding& finding) {
    return finding.category == "candidate_render_api_hook";
  }));
  CHECK(std::any_of(findings.begin(), findings.end(), [](const HeuristicFinding& finding) {
    return finding.category == "candidate_vr_runtime_hook";
  }));
  CHECK(std::any_of(findings.begin(), findings.end(), [](const HeuristicFinding& finding) {
    return finding.category == "candidate_engine_fingerprint";
  }));
  CHECK(std::any_of(findings.begin(), findings.end(), [](const HeuristicFinding& finding) {
    return finding.category == "candidate_vtable_region";
  }));
}

}  // namespace
