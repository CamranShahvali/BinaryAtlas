#include "binaryatlas/output/json_writer.hpp"

#include <nlohmann/json.hpp>

#include "binaryatlas/util/string.hpp"

namespace binaryatlas
{
namespace
{

using json = nlohmann::json;

[[nodiscard]] json addressJson(Address address)
{
  return json{{"value", address}, {"hex", formatHex(address)}};
}

[[nodiscard]] json toJson(const BinaryMetadata& metadata)
{
  return json{
      {"format", toString(metadata.format)},
      {"architecture", toString(metadata.architecture)},
      {"endianness", toString(metadata.endianness)},
      {"format_version", metadata.format_version},
      {"source_path", metadata.source_path},
      {"file_size", metadata.file_size},
      {"entry_point", addressJson(metadata.entry_point)},
      {"abi", metadata.abi},
      {"abi_version", metadata.abi_version},
      {"type", metadata.type},
      {"machine", metadata.machine},
      {"stripped", metadata.stripped},
  };
}

[[nodiscard]] json toJson(const Section& section)
{
  return json{
      {"name", section.name},
      {"range",
       {{"start", addressJson(section.range.start)}, {"end", addressJson(section.range.end)}}},
      {"file_offset", section.file_offset},
      {"file_size", section.file_size},
      {"alignment", section.alignment},
      {"flags", section.flags},
      {"type", section.type},
      {"executable", section.executable},
      {"writable", section.writable},
      {"alloc", section.alloc},
  };
}

[[nodiscard]] json toJson(const Segment& segment)
{
  return json{
      {"type_name", segment.type_name},
      {"range",
       {{"start", addressJson(segment.range.start)}, {"end", addressJson(segment.range.end)}}},
      {"file_offset", segment.file_offset},
      {"file_size", segment.file_size},
      {"memory_size", segment.memory_size},
      {"flags", segment.flags},
      {"type", segment.type},
      {"executable", segment.executable},
      {"writable", segment.writable},
      {"readable", segment.readable},
  };
}

[[nodiscard]] json toJson(const Symbol& symbol)
{
  return json{
      {"name", symbol.name},
      {"section_name", symbol.section_name},
      {"source_table", symbol.source_table},
      {"address", addressJson(symbol.address)},
      {"size", symbol.size},
      {"type", toString(symbol.type)},
      {"binding", toString(symbol.binding)},
      {"defined", symbol.defined},
      {"imported", symbol.imported},
      {"exported", symbol.exported},
  };
}

[[nodiscard]] json toJson(const Relocation& relocation)
{
  return json{
      {"section_name", relocation.section_name},
      {"symbol_name", relocation.symbol_name},
      {"address", addressJson(relocation.address)},
      {"type", relocation.type},
      {"addend", relocation.addend},
      {"has_addend", relocation.has_addend},
  };
}

[[nodiscard]] json toJson(const ExtractedString& extracted)
{
  return json{
      {"section_name", extracted.section_name},
      {"address", addressJson(extracted.address)},
      {"value", extracted.value},
  };
}

[[nodiscard]] json toJson(const MemoryReference& reference)
{
  return json{
      {"instruction_address", addressJson(reference.instruction_address)},
      {"target", addressJson(reference.target)},
      {"resolved", reference.resolved},
      {"rip_relative", reference.rip_relative},
      {"expression", reference.expression},
  };
}

[[nodiscard]] json toJson(const Instruction& instruction)
{
  json bytes = json::array();
  for (std::uint8_t byte : instruction.bytes)
  {
    bytes.push_back(byte);
  }

  json references = json::array();
  for (const MemoryReference& reference : instruction.memory_references)
  {
    references.push_back(toJson(reference));
  }

  json value = {
      {"section_name", instruction.section_name},
      {"address", addressJson(instruction.address)},
      {"size", instruction.size},
      {"bytes", bytes},
      {"mnemonic", instruction.mnemonic},
      {"operands", instruction.operand_text},
      {"groups", instruction.groups},
      {"flow_kind", toString(instruction.flow_kind)},
      {"memory_references", references},
      {"invalid", instruction.invalid},
  };
  if (instruction.branch_target.has_value())
  {
    value["branch_target"] = addressJson(*instruction.branch_target);
  }
  return value;
}

[[nodiscard]] json toJson(const FunctionSeed& seed)
{
  return json{
      {"address", addressJson(seed.address)},
      {"source", seed.source},
      {"label", seed.label},
      {"confidence", seed.confidence},
  };
}

[[nodiscard]] json toJson(const BlockEdge& edge)
{
  json value = {
      {"source", addressJson(edge.source)},
      {"type", toString(edge.type)},
      {"intra_function", edge.intra_function},
  };
  if (edge.target.has_value())
  {
    value["target"] = addressJson(*edge.target);
  }
  return value;
}

[[nodiscard]] json toJson(const BasicBlock& block)
{
  json instructions = json::array();
  for (Address address : block.instruction_addresses)
  {
    instructions.push_back(addressJson(address));
  }

  json edges = json::array();
  for (const BlockEdge& edge : block.outgoing_edges)
  {
    edges.push_back(toJson(edge));
  }

  return json{
      {"start", addressJson(block.start)},
      {"end", addressJson(block.end)},
      {"instruction_addresses", instructions},
      {"edges", edges},
      {"partial", block.partial},
  };
}

[[nodiscard]] json toJson(const CallSite& site)
{
  json value = {
      {"instruction_address", addressJson(site.instruction_address)},
      {"indirect", site.indirect},
      {"resolved", site.resolved},
      {"operand_text", site.operand_text},
  };
  if (site.target.has_value())
  {
    value["target"] = addressJson(*site.target);
  }
  return value;
}

[[nodiscard]] json toJson(const FunctionMetrics& metrics)
{
  return json{
      {"block_count", metrics.block_count},
      {"edge_count", metrics.edge_count},
      {"instruction_count", metrics.instruction_count},
      {"size_bytes", metrics.size_bytes},
      {"loop_count", metrics.loop_count},
      {"connected_components", metrics.connected_components},
      {"max_in_degree", metrics.max_in_degree},
      {"max_out_degree", metrics.max_out_degree},
      {"average_out_degree", metrics.average_out_degree},
      {"call_out_degree", metrics.call_out_degree},
      {"cyclomatic_complexity", metrics.cyclomatic_complexity},
  };
}

[[nodiscard]] json toJson(const RecoveredFunction& function)
{
  json seeds = json::array();
  for (const FunctionSeed& seed : function.seed_reasons)
  {
    seeds.push_back(toJson(seed));
  }

  json blocks = json::array();
  for (const BasicBlock& block : function.blocks)
  {
    blocks.push_back(toJson(block));
  }

  json call_sites = json::array();
  for (const CallSite& call_site : function.call_sites)
  {
    call_sites.push_back(toJson(call_site));
  }

  json callees = json::array();
  for (Address callee : function.callees)
  {
    callees.push_back(addressJson(callee));
  }

  return json{
      {"entry", addressJson(function.entry)},
      {"name", function.name},
      {"seeds", seeds},
      {"blocks", blocks},
      {"call_sites", call_sites},
      {"callees", callees},
      {"referenced_strings", function.referenced_strings},
      {"metrics", toJson(function.metrics)},
      {"partial", function.partial},
      {"ambiguous", function.ambiguous},
  };
}

[[nodiscard]] json toJson(const CallGraphEdge& edge)
{
  return json{
      {"source", addressJson(edge.source)},
      {"target", addressJson(edge.target)},
      {"external", edge.external},
      {"target_name", edge.target_name},
  };
}

[[nodiscard]] json toJson(const ProgramMetrics& metrics)
{
  return json{
      {"function_count", metrics.function_count},
      {"block_count", metrics.block_count},
      {"instruction_count", metrics.instruction_count},
      {"call_edge_count", metrics.call_edge_count},
      {"looping_function_count", metrics.looping_function_count},
      {"dispatcher_candidate_count", metrics.dispatcher_candidate_count},
  };
}

[[nodiscard]] json toJson(const HeuristicFinding& finding)
{
  json value = {
      {"category", finding.category},
      {"confidence", finding.confidence},
      {"evidence", finding.evidence},
      {"explanation", finding.explanation},
  };
  if (finding.address.has_value())
  {
    value["address"] = addressJson(*finding.address);
  }
  if (finding.function.has_value())
  {
    value["function"] = addressJson(*finding.function);
  }
  return value;
}

[[nodiscard]] json disassemblyJson(const DisassemblyResult& disassembly)
{
  json instructions = json::array();
  for (const Instruction& instruction : disassembly.instructions())
  {
    instructions.push_back(toJson(instruction));
  }
  return json{
      {"instruction_count", disassembly.instructions().size()},
      {"invalid_instruction_count", disassembly.invalidInstructionCount()},
      {"instructions", instructions},
  };
}

[[nodiscard]] json analysisJson(const ProgramAnalysis& analysis)
{
  json functions = json::array();
  for (const RecoveredFunction& function : analysis.functions)
  {
    functions.push_back(toJson(function));
  }

  json call_graph = json::array();
  for (const CallGraphEdge& edge : analysis.call_graph)
  {
    call_graph.push_back(toJson(edge));
  }

  json unused = json::array();
  for (const FunctionSeed& seed : analysis.unused_seeds)
  {
    unused.push_back(toJson(seed));
  }

  return json{
      {"functions", functions},
      {"call_graph", call_graph},
      {"metrics", toJson(analysis.metrics)},
      {"unused_seeds", unused},
  };
}

} // namespace

std::string JsonWriter::renderInspect(const BinaryImage& image)
{
  json sections = json::array();
  for (const Section& section : image.sections())
  {
    sections.push_back(toJson(section));
  }

  json segments = json::array();
  for (const Segment& segment : image.segments())
  {
    segments.push_back(toJson(segment));
  }

  json symbols = json::array();
  for (const Symbol& symbol : image.symbols())
  {
    symbols.push_back(toJson(symbol));
  }

  json relocations = json::array();
  for (const Relocation& relocation : image.relocations())
  {
    relocations.push_back(toJson(relocation));
  }

  json strings = json::array();
  for (const ExtractedString& extracted : image.extractedStrings())
  {
    strings.push_back(toJson(extracted));
  }

  return json{
      {"metadata", toJson(image.metadata())},
      {"sections", sections},
      {"segments", segments},
      {"symbols", symbols},
      {"relocations", relocations},
      {"imports", image.imports()},
      {"exports", image.exports()},
      {"strings", strings},
  }
      .dump(2);
}

std::string JsonWriter::renderDisassembly(const DisassemblyResult& disassembly)
{
  return disassemblyJson(disassembly).dump(2);
}

std::string JsonWriter::renderFunctions(const ProgramAnalysis& analysis)
{
  return analysisJson(analysis).dump(2);
}

std::string JsonWriter::renderHeuristics(const std::vector<HeuristicFinding>& findings)
{
  json values = json::array();
  for (const HeuristicFinding& finding : findings)
  {
    values.push_back(toJson(finding));
  }
  return json{{"findings", values}}.dump(2);
}

std::string JsonWriter::renderAnalysis(const AnalysisBundle& bundle)
{
  json value = nlohmann::json::parse(renderInspect(bundle.image));
  value["disassembly"] = nlohmann::json::parse(renderDisassembly(bundle.disassembly));
  value["analysis"] = nlohmann::json::parse(renderFunctions(bundle.analysis));
  value["heuristics"] = nlohmann::json::parse(renderHeuristics(bundle.heuristic_findings));
  return value.dump(2);
}

} // namespace binaryatlas
