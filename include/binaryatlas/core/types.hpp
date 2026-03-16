#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace binaryatlas
{

using Address = std::uint64_t;

struct AddressRange
{
  Address start{};
  Address end{};

  [[nodiscard]] bool contains(Address address) const
  {
    return address >= start && address < end;
  }

  [[nodiscard]] std::uint64_t size() const
  {
    return end >= start ? end - start : 0;
  }
};

enum class BinaryFormat
{
  unknown,
  elf,
  pe,
  macho
};

enum class Architecture
{
  unknown,
  x86_64
};

enum class Endianness
{
  unknown,
  little,
  big
};

enum class SymbolType
{
  unknown,
  no_type,
  function,
  object,
  section,
  file,
  tls
};

enum class SymbolBinding
{
  unknown,
  local,
  global,
  weak
};

struct BinaryMetadata
{
  BinaryFormat format{BinaryFormat::unknown};
  Architecture architecture{Architecture::unknown};
  Endianness endianness{Endianness::unknown};
  std::string format_version;
  std::string source_path;
  std::uint64_t file_size{};
  std::uint64_t entry_point{};
  std::uint32_t abi{};
  std::uint32_t abi_version{};
  std::uint32_t type{};
  std::uint32_t machine{};
  bool stripped{false};
};

struct Section
{
  std::string name;
  AddressRange range{};
  std::uint64_t file_offset{};
  std::uint64_t file_size{};
  std::uint64_t alignment{};
  std::uint64_t flags{};
  std::uint32_t type{};
  bool executable{false};
  bool writable{false};
  bool alloc{false};
};

struct Segment
{
  std::string type_name;
  AddressRange range{};
  std::uint64_t file_offset{};
  std::uint64_t file_size{};
  std::uint64_t memory_size{};
  std::uint32_t flags{};
  std::uint32_t type{};
  bool executable{false};
  bool writable{false};
  bool readable{false};
};

struct Symbol
{
  std::string name;
  std::string section_name;
  std::string source_table;
  Address address{};
  std::uint64_t size{};
  SymbolType type{SymbolType::unknown};
  SymbolBinding binding{SymbolBinding::unknown};
  bool defined{false};
  bool imported{false};
  bool exported{false};
};

struct Relocation
{
  std::string section_name;
  std::string symbol_name;
  Address address{};
  std::uint32_t type{};
  std::int64_t addend{};
  bool has_addend{false};
};

struct ExtractedString
{
  std::string section_name;
  Address address{};
  std::string value;
};

enum class InstructionFlowKind
{
  invalid,
  normal,
  call_direct,
  call_indirect,
  jump_conditional,
  jump_unconditional,
  jump_indirect,
  return_flow,
  trap
};

struct MemoryReference
{
  Address instruction_address{};
  Address target{};
  bool resolved{false};
  bool rip_relative{false};
  std::string expression;
};

enum class BlockEdgeType
{
  fallthrough,
  true_branch,
  false_branch,
  jump,
  call_fallthrough,
  unresolved_indirect,
  return_edge
};

[[nodiscard]] std::string toString(BinaryFormat format);
[[nodiscard]] std::string toString(Architecture architecture);
[[nodiscard]] std::string toString(Endianness endianness);
[[nodiscard]] std::string toString(SymbolType type);
[[nodiscard]] std::string toString(SymbolBinding binding);
[[nodiscard]] std::string toString(InstructionFlowKind flow_kind);
[[nodiscard]] std::string toString(BlockEdgeType edge_type);

} // namespace binaryatlas
