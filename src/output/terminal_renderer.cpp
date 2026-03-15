#include "binaryatlas/output/terminal_renderer.hpp"

#include <algorithm>
#include <sstream>

#include "binaryatlas/util/string.hpp"

namespace binaryatlas
{
namespace
{

template <typename Range>
void renderList(std::ostringstream& stream, const Range& items, std::string_view prefix)
{
  for (const auto& item : items)
  {
    stream << prefix << item << '\n';
  }
}

}  // namespace

std::string TerminalRenderer::renderInspect(const BinaryImage& image, bool verbose)
{
  std::ostringstream output;
  output << "BinaryAtlas inspect\n";
  output << "  Path: " << image.sourcePath().string() << '\n';
  output << "  Format: " << toString(image.metadata().format) << " / "
         << toString(image.metadata().architecture) << '\n';
  output << "  Entry: " << formatHex(image.metadata().entry_point) << '\n';
  output << "  Sections: " << image.sections().size() << '\n';
  output << "  Symbols: " << image.symbols().size() << '\n';
  output << "  Relocations: " << image.relocations().size() << '\n';
  output << "  Imports: " << image.imports().size() << '\n';

  if (verbose)
  {
    output << "\nSections\n";
    for (const Section& section : image.sections())
    {
      output << "  " << section.name << " " << formatHex(section.range.start) << " - "
             << formatHex(section.range.end) << " size=" << section.file_size
             << " exec=" << (section.executable ? "yes" : "no") << '\n';
    }

    output << "\nSymbols\n";
    const std::size_t limit = std::min<std::size_t>(image.symbols().size(), 32);
    for (std::size_t index = 0; index < limit; ++index)
    {
      const Symbol& symbol = image.symbols()[index];
      output << "  " << formatHex(symbol.address) << " " << symbol.name << " "
             << toString(symbol.type) << '\n';
    }
  }

  return output.str();
}

std::string TerminalRenderer::renderDisassembly(
    const DisassemblyResult& disassembly,
    bool verbose)
{
  std::ostringstream output;
  output << "BinaryAtlas disasm\n";
  output << "  Instructions: " << disassembly.instructions().size() << '\n';
  output << "  Invalid bytes: " << disassembly.invalidInstructionCount() << '\n';

  const std::size_t limit = verbose ? disassembly.instructions().size()
                                    : std::min<std::size_t>(disassembly.instructions().size(), 40);
  for (std::size_t index = 0; index < limit; ++index)
  {
    const Instruction& instruction = disassembly.instructions()[index];
    output << "  " << formatHex(instruction.address) << ": " << instruction.mnemonic;
    if (!instruction.operand_text.empty())
    {
      output << ' ' << instruction.operand_text;
    }
    output << " [" << toString(instruction.flow_kind) << "]\n";
  }

  return output.str();
}

std::string TerminalRenderer::renderFunctions(const ProgramAnalysis& analysis, bool verbose)
{
  std::ostringstream output;
  output << "Recovered functions: " << analysis.functions.size() << '\n';
  output << "Call graph edges: " << analysis.call_graph.size() << '\n';

  for (const RecoveredFunction& function : analysis.functions)
  {
    output << "  " << formatHex(function.entry) << " " << function.name
           << " blocks=" << function.metrics.block_count
           << " complexity=" << function.metrics.cyclomatic_complexity
           << " calls=" << function.metrics.call_out_degree
           << " partial=" << (function.partial ? "yes" : "no") << '\n';

    if (verbose)
    {
      for (const BasicBlock& block : function.blocks)
      {
        output << "    block " << formatHex(block.start) << " -> " << formatHex(block.end)
               << " insns=" << block.instruction_addresses.size() << '\n';
      }
    }
  }

  return output.str();
}

std::string TerminalRenderer::renderCfg(const RecoveredFunction& function)
{
  std::ostringstream output;
  output << "CFG for " << function.name << " (" << formatHex(function.entry) << ")\n";
  for (const BasicBlock& block : function.blocks)
  {
    output << "  Block " << formatHex(block.start) << " -> " << formatHex(block.end) << '\n';
    for (const BlockEdge& edge : block.outgoing_edges)
    {
      output << "    edge " << toString(edge.type);
      if (edge.target.has_value())
      {
        output << " -> " << formatHex(*edge.target);
      }
      output << '\n';
    }
  }
  return output.str();
}

std::string TerminalRenderer::renderCallGraph(const ProgramAnalysis& analysis)
{
  std::ostringstream output;
  output << "Call graph\n";
  for (const CallGraphEdge& edge : analysis.call_graph)
  {
    output << "  " << formatHex(edge.source) << " -> " << formatHex(edge.target);
    if (!edge.target_name.empty())
    {
      output << " (" << edge.target_name << ")";
    }
    if (edge.external)
    {
      output << " external";
    }
    output << '\n';
  }
  return output.str();
}

std::string TerminalRenderer::renderHeuristics(
    const std::vector<HeuristicFinding>& findings,
    bool verbose)
{
  std::ostringstream output;
  output << "Heuristic findings: " << findings.size() << '\n';
  for (const HeuristicFinding& finding : findings)
  {
    output << "  [" << finding.category << "] confidence=" << finding.confidence;
    if (finding.function.has_value())
    {
      output << " function=" << formatHex(*finding.function);
    }
    else if (finding.address.has_value())
    {
      output << " address=" << formatHex(*finding.address);
    }
    output << '\n';
    output << "    " << finding.explanation << '\n';
    if (verbose)
    {
      renderList(output, finding.evidence, "    evidence: ");
    }
  }
  return output.str();
}

std::string TerminalRenderer::renderAnalysis(const AnalysisBundle& bundle, bool verbose)
{
  std::ostringstream output;
  output << renderInspect(bundle.image, false) << '\n';
  output << renderFunctions(bundle.analysis, verbose) << '\n';
  output << renderHeuristics(bundle.heuristic_findings, verbose);
  return output.str();
}

}  // namespace binaryatlas
