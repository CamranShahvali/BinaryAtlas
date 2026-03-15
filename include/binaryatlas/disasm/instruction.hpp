#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "binaryatlas/core/result.hpp"
#include "binaryatlas/core/types.hpp"

namespace binaryatlas
{

struct Instruction
{
  std::string section_name;
  Address address {};
  std::uint8_t size {};
  std::vector<std::uint8_t> bytes;
  std::string mnemonic;
  std::string operand_text;
  std::vector<std::string> groups;
  InstructionFlowKind flow_kind {InstructionFlowKind::invalid};
  std::optional<Address> branch_target;
  std::vector<MemoryReference> memory_references;
  bool invalid {false};
};

struct DisassemblyOptions
{
  std::optional<std::string> section_name;
  std::optional<Address> start_address;
  std::optional<Address> end_address;
};

class DisassemblyResult
{
public:
  [[nodiscard]] const std::vector<Instruction>& instructions() const;
  [[nodiscard]] const std::map<Address, std::size_t>& index() const;
  [[nodiscard]] const Instruction* find(Address address) const;
  [[nodiscard]] std::vector<const Instruction*> inAddressRange(
      Address start,
      Address end) const;
  [[nodiscard]] std::vector<const Instruction*> inSection(std::string_view section_name) const;
  [[nodiscard]] std::size_t invalidInstructionCount() const;

  void addInstruction(Instruction instruction);

private:
  std::vector<Instruction> instructions_;
  std::map<Address, std::size_t> address_index_;
  std::size_t invalid_instruction_count_ {};
};

}  // namespace binaryatlas
