#include "binaryatlas/core/types.hpp"

namespace binaryatlas
{

std::string toString(BinaryFormat format)
{
  switch (format)
  {
  case BinaryFormat::elf:
    return "elf";
  case BinaryFormat::pe:
    return "pe";
  case BinaryFormat::macho:
    return "macho";
  case BinaryFormat::unknown:
    return "unknown";
  }
  return "unknown";
}

std::string toString(Architecture architecture)
{
  switch (architecture)
  {
  case Architecture::x86_64:
    return "x86_64";
  case Architecture::unknown:
    return "unknown";
  }
  return "unknown";
}

std::string toString(Endianness endianness)
{
  switch (endianness)
  {
  case Endianness::little:
    return "little";
  case Endianness::big:
    return "big";
  case Endianness::unknown:
    return "unknown";
  }
  return "unknown";
}

std::string toString(SymbolType type)
{
  switch (type)
  {
  case SymbolType::no_type:
    return "none";
  case SymbolType::function:
    return "function";
  case SymbolType::object:
    return "object";
  case SymbolType::section:
    return "section";
  case SymbolType::file:
    return "file";
  case SymbolType::tls:
    return "tls";
  case SymbolType::unknown:
    return "unknown";
  }
  return "unknown";
}

std::string toString(SymbolBinding binding)
{
  switch (binding)
  {
  case SymbolBinding::local:
    return "local";
  case SymbolBinding::global:
    return "global";
  case SymbolBinding::weak:
    return "weak";
  case SymbolBinding::unknown:
    return "unknown";
  }
  return "unknown";
}

std::string toString(InstructionFlowKind flow_kind)
{
  switch (flow_kind)
  {
  case InstructionFlowKind::invalid:
    return "invalid";
  case InstructionFlowKind::normal:
    return "normal";
  case InstructionFlowKind::call_direct:
    return "call_direct";
  case InstructionFlowKind::call_indirect:
    return "call_indirect";
  case InstructionFlowKind::jump_conditional:
    return "jump_conditional";
  case InstructionFlowKind::jump_unconditional:
    return "jump_unconditional";
  case InstructionFlowKind::jump_indirect:
    return "jump_indirect";
  case InstructionFlowKind::return_flow:
    return "return";
  case InstructionFlowKind::trap:
    return "trap";
  }
  return "unknown";
}

std::string toString(BlockEdgeType edge_type)
{
  switch (edge_type)
  {
  case BlockEdgeType::fallthrough:
    return "fallthrough";
  case BlockEdgeType::true_branch:
    return "true_branch";
  case BlockEdgeType::false_branch:
    return "false_branch";
  case BlockEdgeType::jump:
    return "jump";
  case BlockEdgeType::call_fallthrough:
    return "call_fallthrough";
  case BlockEdgeType::unresolved_indirect:
    return "unresolved_indirect";
  case BlockEdgeType::return_edge:
    return "return";
  }
  return "unknown";
}

} // namespace binaryatlas
