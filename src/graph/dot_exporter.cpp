#include "binaryatlas/graph/dot_exporter.hpp"

#include <sstream>

#include "binaryatlas/util/string.hpp"

namespace binaryatlas
{

std::string DotExporter::exportFunctionCfg(const RecoveredFunction& function)
{
  std::ostringstream dot;
  dot << "digraph \"" << function.name << "_cfg\" {\n";
  dot << "  rankdir=TB;\n";
  dot << "  node [shape=box,fontname=\"Courier\"];\n";

  for (const BasicBlock& block : function.blocks)
  {
    dot << "  \"" << formatHex(block.start) << "\" [label=\"" << formatHex(block.start)
        << "\\ninsns=" << block.instruction_addresses.size() << "\"];\n";
  }

  for (const BasicBlock& block : function.blocks)
  {
    for (const BlockEdge& edge : block.outgoing_edges)
    {
      if (!edge.target.has_value())
      {
        continue;
      }
      dot << "  \"" << formatHex(block.start) << "\" -> \"" << formatHex(*edge.target)
          << "\" [label=\"" << toString(edge.type) << "\"";
      if (!edge.intra_function)
      {
        dot << ",style=dashed";
      }
      dot << "];\n";
    }
  }

  dot << "}\n";
  return dot.str();
}

std::string DotExporter::exportCallGraph(const ProgramAnalysis& analysis)
{
  std::ostringstream dot;
  dot << "digraph \"callgraph\" {\n";
  dot << "  rankdir=LR;\n";
  dot << "  node [shape=box,fontname=\"Courier\"];\n";

  for (const RecoveredFunction& function : analysis.functions)
  {
    dot << "  \"" << formatHex(function.entry) << "\" [label=\"" << function.name << "\\n"
        << formatHex(function.entry) << "\"];\n";
  }

  for (const CallGraphEdge& edge : analysis.call_graph)
  {
    dot << "  \"" << formatHex(edge.source) << "\" -> \"" << formatHex(edge.target) << "\"";
    dot << " [label=\"" << (edge.target_name.empty() ? formatHex(edge.target) : edge.target_name)
        << "\"";
    if (edge.external)
    {
      dot << ",style=dashed";
    }
    dot << "];\n";
  }

  dot << "}\n";
  return dot.str();
}

} // namespace binaryatlas
