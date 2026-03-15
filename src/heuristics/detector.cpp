#include "binaryatlas/heuristics/detector.hpp"

#include <algorithm>
#include <cstring>
#include <set>
#include <sstream>

#include "binaryatlas/util/string.hpp"

namespace binaryatlas
{
namespace
{

[[nodiscard]] double clampConfidence(double value)
{
  return std::clamp(value, 0.0, 1.0);
}

[[nodiscard]] std::vector<std::string> collectMatches(
    const std::vector<std::string>& haystacks,
    const std::vector<std::string>& keywords)
{
  std::set<std::string> matches;
  for (const std::string& value : haystacks)
  {
    if (const std::optional<std::string> keyword = firstMatchingKeyword(value, keywords))
    {
      matches.insert(*keyword);
    }
  }
  return {matches.begin(), matches.end()};
}

[[nodiscard]] std::uint64_t readLittleEndianPointer(const std::uint8_t* bytes)
{
  std::uint64_t value = 0;
  std::memcpy(&value, bytes, sizeof(value));
  return value;
}

[[nodiscard]] std::vector<HeuristicFinding> detectVtableRegions(const BinaryImage& image)
{
  std::vector<HeuristicFinding> findings;

  for (const Section& section : image.sections())
  {
    if (section.executable || !section.alloc || section.file_size < 24 || section.file_offset >= image.fileBytes().size())
    {
      continue;
    }

    const std::size_t section_offset = static_cast<std::size_t>(section.file_offset);
    const std::size_t available = static_cast<std::size_t>(
        std::min<std::uint64_t>(section.file_size, image.fileBytes().size() - section.file_offset));

    std::size_t cursor = 0;
    while (cursor + sizeof(std::uint64_t) <= available)
    {
      std::size_t run_length = 0;
      while (cursor + ((run_length + 1) * sizeof(std::uint64_t)) <= available)
      {
        const std::uint64_t pointer = readLittleEndianPointer(
            image.fileBytes().data() + section_offset + cursor + (run_length * sizeof(std::uint64_t)));
        if (!image.isExecutableAddress(pointer))
        {
          break;
        }
        ++run_length;
      }

      if (run_length >= 3)
      {
        const Address region_address = section.range.start + static_cast<Address>(cursor);
        HeuristicFinding finding;
        finding.category = "candidate_vtable_region";
        finding.address = region_address;
        finding.confidence = clampConfidence(0.55 + (0.08 * static_cast<double>(run_length - 3)));
        finding.evidence = {
            std::to_string(run_length) + " consecutive executable pointers",
            "section=" + section.name,
            "address=" + formatHex(region_address)};
        finding.explanation =
            "A non-executable data region contains multiple consecutive pointers into executable code, "
            "which often appears in C++ virtual table layouts.";
        findings.push_back(std::move(finding));
        cursor += run_length * sizeof(std::uint64_t);
        continue;
      }

      cursor += sizeof(std::uint64_t);
    }
  }

  return findings;
}

[[nodiscard]] std::vector<std::string> functionTextFeatures(const RecoveredFunction& function)
{
  std::vector<std::string> features;
  features.push_back(function.name);
  features.insert(features.end(), function.referenced_strings.begin(), function.referenced_strings.end());
  for (const CallSite& call_site : function.call_sites)
  {
    features.push_back(call_site.operand_text);
  }
  return features;
}

[[nodiscard]] bool isRuntimeSupportFunction(std::string_view name)
{
  return name == "_init" || name == "_start" || name == "_fini" ||
         name == "deregister_tm_clones" || name == "register_tm_clones" ||
         name == "frame_dummy" || name == "__do_global_dtors_aux";
}

[[nodiscard]] bool hasIndirectDispatchShape(
    const RecoveredFunction& function,
    const DisassemblyResult& disassembly)
{
  std::size_t indirect_call_count = 0;
  for (const CallSite& site : function.call_sites)
  {
    if (!site.indirect)
    {
      continue;
    }

    ++indirect_call_count;
    if (site.operand_text.find('[') != std::string::npos && !icontains(site.operand_text, "rip"))
    {
      return true;
    }
  }

  if (indirect_call_count == 0 || isRuntimeSupportFunction(function.name))
  {
    return false;
  }

  std::size_t vtable_like_loads = 0;
  for (const BasicBlock& block : function.blocks)
  {
    for (Address address : block.instruction_addresses)
    {
      const Instruction* instruction = disassembly.find(address);
      if (instruction == nullptr)
      {
        continue;
      }

      if (instruction->operand_text.find("qword ptr [") != std::string::npos &&
          !icontains(instruction->operand_text, "rip"))
      {
        ++vtable_like_loads;
      }
    }
  }

  return vtable_like_loads >= 2;
}

[[nodiscard]] std::size_t unresolvedIndirectJumpCount(const RecoveredFunction& function)
{
  std::size_t count = 0;
  for (const BasicBlock& block : function.blocks)
  {
    for (const BlockEdge& edge : block.outgoing_edges)
    {
      if (edge.type == BlockEdgeType::unresolved_indirect)
      {
        ++count;
      }
    }
  }
  return count;
}

}  // namespace

std::vector<HeuristicFinding> HeuristicDetector::analyze(
    const BinaryImage& image,
    const DisassemblyResult& disassembly,
    const ProgramAnalysis& analysis,
    const HeuristicOptions& options) const
{
  std::vector<HeuristicFinding> findings = detectVtableRegions(image);
  const bool has_vtable_region = std::any_of(findings.begin(), findings.end(), [](const HeuristicFinding& finding) {
    return finding.category == "candidate_vtable_region";
  });

  std::vector<std::string> binary_text_features = image.imports();
  for (const Symbol& symbol : image.symbols())
  {
    if (!symbol.name.empty())
    {
      binary_text_features.push_back(symbol.name);
    }
  }
  for (const ExtractedString& extracted : image.extractedStrings())
  {
    binary_text_features.push_back(extracted.value);
  }

  const std::vector<std::string> engine_matches =
      collectMatches(binary_text_features, options.engine_keywords);
  const std::vector<std::string> vr_binary_matches =
      collectMatches(binary_text_features, options.vr_keywords);
  if (!engine_matches.empty())
  {
    HeuristicFinding finding;
    finding.category = "candidate_engine_fingerprint";
    finding.confidence = clampConfidence(0.45 + (0.1 * static_cast<double>(engine_matches.size())));
    finding.evidence = engine_matches;
    finding.explanation =
        "Binary-level symbols or embedded strings match known engine identifiers. This is a coarse "
        "fingerprint rather than a definitive identification.";
    findings.push_back(std::move(finding));
  }
  if (!vr_binary_matches.empty())
  {
    HeuristicFinding finding;
    finding.category = "candidate_vr_runtime_hook";
    finding.confidence = clampConfidence(0.4 + (0.1 * static_cast<double>(vr_binary_matches.size())));
    finding.evidence = vr_binary_matches;
    finding.explanation =
        "Binary-level strings or symbols match VR runtime terminology. This is useful triage context, "
        "but the specific runtime touch points still need manual confirmation.";
    findings.push_back(std::move(finding));
  }

  for (const RecoveredFunction& function : analysis.functions)
  {
    const std::vector<std::string> features = functionTextFeatures(function);

    if (function.metrics.cyclomatic_complexity >= options.high_complexity_threshold)
    {
      HeuristicFinding finding;
      finding.category = "high_complexity_function";
      finding.address = function.entry;
      finding.function = function.entry;
      finding.confidence = clampConfidence(
          0.5 + (0.04 * static_cast<double>(
                           function.metrics.cyclomatic_complexity - options.high_complexity_threshold)));
      finding.evidence = {
          "function=" + function.name,
          "cyclomatic_complexity=" + std::to_string(function.metrics.cyclomatic_complexity),
          "blocks=" + std::to_string(function.metrics.block_count)};
      finding.explanation =
          "The recovered control-flow graph exceeds the configured complexity threshold and is worth "
          "prioritizing during manual review.";
      findings.push_back(std::move(finding));
    }

    if (function.metrics.max_out_degree >= options.dispatcher_out_degree_threshold ||
        function.metrics.call_out_degree >= options.dispatcher_out_degree_threshold)
    {
      HeuristicFinding finding;
      finding.category = "dispatcher_like_function";
      finding.address = function.entry;
      finding.function = function.entry;
      finding.confidence = clampConfidence(
          0.55 + (0.05 * static_cast<double>(
                            std::max(function.metrics.max_out_degree, function.metrics.call_out_degree) -
                            options.dispatcher_out_degree_threshold)));
      finding.evidence = {
          "function=" + function.name,
          "max_block_out_degree=" + std::to_string(function.metrics.max_out_degree),
          "call_out_degree=" + std::to_string(function.metrics.call_out_degree)};
      finding.explanation =
          "This function fans out to many successors or callees, which is a common dispatcher or command "
          "routing shape.";
      findings.push_back(std::move(finding));
    }

    const std::vector<std::string> graphics_matches =
        collectMatches(features, options.graphics_keywords);
    const std::vector<std::string> vr_matches = collectMatches(features, options.vr_keywords);
    const std::vector<std::string> engine_feature_matches =
        collectMatches(features, options.engine_keywords);

    if (hasIndirectDispatchShape(function, disassembly))
    {
      HeuristicFinding finding;
      finding.category = "candidate_virtual_dispatch";
      finding.address = function.entry;
      finding.function = function.entry;
      finding.confidence = clampConfidence(
          0.55 + (0.08 * static_cast<double>(std::count_if(
                            function.call_sites.begin(),
                            function.call_sites.end(),
                            [](const CallSite& site) { return site.indirect; }))) +
          (has_vtable_region ? 0.1 : 0.0));
      finding.evidence = {"function=" + function.name};
      for (const CallSite& site : function.call_sites)
      {
        if (site.indirect)
        {
          finding.evidence.push_back("indirect_call=" + site.operand_text);
        }
      }
      finding.explanation =
          "The function performs indirect memory-based calls through non-RIP operands. That pattern often "
          "appears in C++ virtual dispatch or callback tables, but can also come from generic function pointers.";
      findings.push_back(std::move(finding));
    }

    const std::size_t jump_table_edges = unresolvedIndirectJumpCount(function);
    if (jump_table_edges > 0 && function.metrics.block_count >= 4)
    {
      HeuristicFinding finding;
      finding.category = "candidate_jump_table";
      finding.address = function.entry;
      finding.function = function.entry;
      finding.confidence = clampConfidence(0.45 + (0.1 * static_cast<double>(jump_table_edges)));
      finding.evidence = {
          "function=" + function.name,
          "unresolved_indirect_edges=" + std::to_string(jump_table_edges),
          "blocks=" + std::to_string(function.metrics.block_count)};
      finding.explanation =
          "An indirect jump terminator was recovered inside a multi-block function. This often corresponds "
          "to a switch jump table, but unresolved dispatchers and hand-written assembly can look similar.";
      findings.push_back(std::move(finding));
    }

    if (!graphics_matches.empty())
    {
      const bool hookish_name = icontains(function.name, "present") || icontains(function.name, "swap") ||
                                icontains(function.name, "hook") || icontains(function.name, "render");

      HeuristicFinding finding;
      finding.category = "candidate_render_api_hook";
      finding.address = function.entry;
      finding.function = function.entry;
      finding.confidence =
          clampConfidence(0.45 + (0.08 * static_cast<double>(graphics_matches.size())) + (hookish_name ? 0.15 : 0.0));
      finding.evidence = graphics_matches;
      finding.evidence.push_back("function=" + function.name);
      finding.explanation =
          "Function-local names or referenced strings match graphics-related APIs or terminology. This is "
          "suggestive of render-path integration or hook-adjacent logic, not proof of a stable hook point.";
      findings.push_back(std::move(finding));
    }

    if (!vr_matches.empty())
    {
      HeuristicFinding finding;
      finding.category = "candidate_vr_runtime_hook";
      finding.address = function.entry;
      finding.function = function.entry;
      finding.confidence = clampConfidence(0.45 + (0.1 * static_cast<double>(vr_matches.size())));
      finding.evidence = vr_matches;
      finding.evidence.push_back("function=" + function.name);
      finding.explanation =
          "Function-local strings or names match VR runtime terminology. This is a heuristic signal for XR "
          "integration or interception points.";
      findings.push_back(std::move(finding));
    }

    if (!engine_feature_matches.empty())
    {
      HeuristicFinding finding;
      finding.category = "candidate_engine_fingerprint";
      finding.address = function.entry;
      finding.function = function.entry;
      finding.confidence = clampConfidence(0.35 + (0.1 * static_cast<double>(engine_feature_matches.size())));
      finding.evidence = engine_feature_matches;
      finding.evidence.push_back("function=" + function.name);
      finding.explanation =
          "The function references engine-related strings or symbols. The match is useful for triage but may "
          "be indirect or incidental.";
      findings.push_back(std::move(finding));
    }
  }

  std::sort(findings.begin(), findings.end(), [](const HeuristicFinding& left, const HeuristicFinding& right) {
    if (left.category != right.category)
    {
      return left.category < right.category;
    }
    if (left.function != right.function)
    {
      return left.function.value_or(0) < right.function.value_or(0);
    }
    return left.address.value_or(0) < right.address.value_or(0);
  });

  return findings;
}

}  // namespace binaryatlas
