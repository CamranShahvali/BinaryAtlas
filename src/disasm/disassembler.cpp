#include "binaryatlas/disasm/disassembler.hpp"

#include <algorithm>
#include <capstone/capstone.h>
#include <memory>
#include <sstream>

#include "binaryatlas/core/error.hpp"
#include "binaryatlas/util/arithmetic.hpp"

namespace binaryatlas
{
namespace
{

class CapstoneHandle
{
public:
  CapstoneHandle()
  {
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle_) != CS_ERR_OK)
    {
      handle_ = 0;
      return;
    }

    cs_option(handle_, CS_OPT_DETAIL, CS_OPT_ON);
  }

  ~CapstoneHandle()
  {
    if (handle_ != 0)
    {
      cs_close(&handle_);
    }
  }

  [[nodiscard]] csh get() const
  {
    return handle_;
  }

  [[nodiscard]] bool valid() const
  {
    return handle_ != 0;
  }

private:
  csh handle_ {};
};

[[nodiscard]] bool isUnconditionalJump(unsigned int instruction_id)
{
  return instruction_id == X86_INS_JMP || instruction_id == X86_INS_LJMP;
}

[[nodiscard]] bool hasGroup(const cs_insn& instruction, std::uint8_t group)
{
  if (instruction.detail == nullptr)
  {
    return false;
  }

  for (std::uint8_t index = 0; index < instruction.detail->groups_count; ++index)
  {
    if (instruction.detail->groups[index] == group)
    {
      return true;
    }
  }
  return false;
}

[[nodiscard]] InstructionFlowKind classifyInstruction(const cs_insn& instruction)
{
  const cs_detail* detail = instruction.detail;
  if (detail == nullptr)
  {
    return InstructionFlowKind::normal;
  }

  if (hasGroup(instruction, CS_GRP_RET))
  {
    return InstructionFlowKind::return_flow;
  }
  if (hasGroup(instruction, CS_GRP_INT) || hasGroup(instruction, CS_GRP_IRET))
  {
    return InstructionFlowKind::trap;
  }
  if (hasGroup(instruction, CS_GRP_CALL))
  {
    if (detail->x86.op_count > 0 && detail->x86.operands[0].type == X86_OP_IMM)
    {
      return InstructionFlowKind::call_direct;
    }
    return InstructionFlowKind::call_indirect;
  }
  if (hasGroup(instruction, CS_GRP_JUMP))
  {
    if (detail->x86.op_count > 0 && detail->x86.operands[0].type == X86_OP_IMM)
    {
      return isUnconditionalJump(instruction.id) ? InstructionFlowKind::jump_unconditional
                                                 : InstructionFlowKind::jump_conditional;
    }
    return InstructionFlowKind::jump_indirect;
  }

  if (instruction.id == X86_INS_HLT || instruction.id == X86_INS_UD2)
  {
    return InstructionFlowKind::trap;
  }

  return InstructionFlowKind::normal;
}

[[nodiscard]] std::optional<Address> directBranchTarget(const cs_insn& instruction)
{
  const cs_detail* detail = instruction.detail;
  if (detail == nullptr || detail->x86.op_count == 0)
  {
    return std::nullopt;
  }

  const cs_x86_op& operand = detail->x86.operands[0];
  if (operand.type == X86_OP_IMM)
  {
    return static_cast<Address>(operand.imm);
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::string> groupNames(csh handle, const cs_insn& instruction)
{
  std::vector<std::string> groups;
  if (instruction.detail == nullptr)
  {
    return groups;
  }

  groups.reserve(instruction.detail->groups_count);
  for (uint8_t index = 0; index < instruction.detail->groups_count; ++index)
  {
    const char* name = cs_group_name(handle, instruction.detail->groups[index]);
    if (name != nullptr)
    {
      groups.emplace_back(name);
    }
  }
  return groups;
}

[[nodiscard]] std::vector<MemoryReference> memoryReferences(
    const cs_insn& instruction,
    const BinaryImage* image)
{
  std::vector<MemoryReference> references;
  const cs_detail* detail = instruction.detail;
  if (detail == nullptr)
  {
    return references;
  }

  for (uint8_t index = 0; index < detail->x86.op_count; ++index)
  {
    const cs_x86_op& operand = detail->x86.operands[index];
    if (operand.type == X86_OP_MEM)
    {
      MemoryReference reference;
      reference.instruction_address = instruction.address;
      reference.expression = instruction.op_str;

      if (operand.mem.base == X86_REG_RIP)
      {
        const auto base = static_cast<std::int64_t>(instruction.address) +
                          static_cast<std::int64_t>(instruction.size);
        const auto target = base + operand.mem.disp;
        if (target >= 0)
        {
          reference.target = static_cast<Address>(target);
          reference.resolved = true;
          reference.rip_relative = true;
        }
      }
      else if (operand.mem.base == X86_REG_INVALID && operand.mem.index == X86_REG_INVALID)
      {
        reference.target = static_cast<Address>(operand.mem.disp);
        reference.resolved = operand.mem.disp >= 0;
      }

      if (reference.resolved)
      {
        references.push_back(std::move(reference));
      }
    }
    else if (operand.type == X86_OP_IMM && image != nullptr)
    {
      const auto section = image->findSectionContaining(static_cast<Address>(operand.imm));
      if (section.has_value())
      {
        references.push_back(
            {instruction.address,
             static_cast<Address>(operand.imm),
             true,
             false,
             instruction.op_str});
      }
    }
  }

  return references;
}

[[nodiscard]] Instruction buildInstruction(
    csh handle,
    const cs_insn& decoded,
    const std::string& section_name,
    const BinaryImage* image)
{
  Instruction instruction;
  instruction.section_name = section_name;
  instruction.address = decoded.address;
  instruction.size = static_cast<std::uint8_t>(decoded.size);
  instruction.bytes.assign(decoded.bytes, decoded.bytes + decoded.size);
  instruction.mnemonic = decoded.mnemonic;
  instruction.operand_text = decoded.op_str;
  instruction.groups = groupNames(handle, decoded);
  instruction.flow_kind = classifyInstruction(decoded);
  instruction.branch_target = directBranchTarget(decoded);
  instruction.memory_references = memoryReferences(decoded, image);
  instruction.invalid = false;
  return instruction;
}

[[nodiscard]] Instruction buildInvalidInstruction(
    Address address,
    std::uint8_t value,
    const std::string& section_name)
{
  Instruction instruction;
  instruction.section_name = section_name;
  instruction.address = address;
  instruction.size = 1;
  instruction.bytes = {value};
  instruction.mnemonic = "db";
  std::ostringstream operands;
  operands << "0x" << std::hex << static_cast<unsigned int>(value);
  instruction.operand_text = operands.str();
  instruction.flow_kind = InstructionFlowKind::invalid;
  instruction.invalid = true;
  return instruction;
}

[[nodiscard]] Result<DisassemblyResult> disassembleBufferInternal(
    const std::vector<std::uint8_t>& bytes,
    Address base_address,
    const std::string& section_name,
    const BinaryImage* image)
{
  CapstoneHandle handle;
  if (!handle.valid())
  {
    return Result<DisassemblyResult>::failure(Error::disassembly("failed to initialize Capstone"));
  }

  DisassemblyResult result;
  auto deleter = [](cs_insn* ptr) {
    if (ptr != nullptr)
    {
      cs_free(ptr, 1);
    }
  };
  std::unique_ptr<cs_insn, decltype(deleter)> instruction(cs_malloc(handle.get()), deleter);

  if (!instruction)
  {
    return Result<DisassemblyResult>::failure(Error::disassembly("failed to allocate Capstone instruction"));
  }

  const std::uint8_t* code = bytes.data();
  std::size_t remaining = bytes.size();
  std::uint64_t address = base_address;

  while (remaining > 0)
  {
    if (cs_disasm_iter(handle.get(), &code, &remaining, &address, instruction.get()))
    {
      result.addInstruction(buildInstruction(handle.get(), *instruction, section_name, image));
      continue;
    }

    result.addInstruction(buildInvalidInstruction(address, *code, section_name));
    ++code;
    --remaining;
    ++address;
  }

  return Result<DisassemblyResult>::success(std::move(result));
}

}  // namespace

const std::vector<Instruction>& DisassemblyResult::instructions() const
{
  return instructions_;
}

const std::map<Address, std::size_t>& DisassemblyResult::index() const
{
  return address_index_;
}

const Instruction* DisassemblyResult::find(Address address) const
{
  const auto iterator = address_index_.find(address);
  if (iterator == address_index_.end())
  {
    return nullptr;
  }
  return &instructions_[iterator->second];
}

std::vector<const Instruction*> DisassemblyResult::inAddressRange(Address start, Address end) const
{
  std::vector<const Instruction*> instructions;
  for (auto iterator = address_index_.lower_bound(start); iterator != address_index_.end() && iterator->first < end;
       ++iterator)
  {
    instructions.push_back(&instructions_[iterator->second]);
  }
  return instructions;
}

std::vector<const Instruction*> DisassemblyResult::inSection(std::string_view section_name) const
{
  std::vector<const Instruction*> instructions;
  for (const Instruction& instruction : instructions_)
  {
    if (instruction.section_name == section_name)
    {
      instructions.push_back(&instruction);
    }
  }
  return instructions;
}

std::size_t DisassemblyResult::invalidInstructionCount() const
{
  return invalid_instruction_count_;
}

void DisassemblyResult::addInstruction(Instruction instruction)
{
  if (instruction.invalid)
  {
    ++invalid_instruction_count_;
  }
  address_index_[instruction.address] = instructions_.size();
  instructions_.push_back(std::move(instruction));
}

Result<DisassemblyResult> Disassembler::disassemble(
    const BinaryImage& image,
    const DisassemblyOptions& options) const
{
  DisassemblyResult result;

  for (const Section& section : image.sections())
  {
    if (!section.executable || section.file_size == 0)
    {
      continue;
    }

    if (options.section_name.has_value() && section.name != *options.section_name)
    {
      continue;
    }

    if (section.file_offset > image.fileBytes().size())
    {
      return Result<DisassemblyResult>::failure(
          Error::disassembly("executable section extends beyond file contents"));
    }

    const Address section_start = section.range.start;
    const Address section_end = section.range.end;
    const Address requested_start = std::max(section_start, options.start_address.value_or(section_start));
    const Address requested_end = std::min(section_end, options.end_address.value_or(section_end));

    if (requested_start >= requested_end)
    {
      continue;
    }

    const std::size_t start_offset = static_cast<std::size_t>(requested_start - section_start);
    const std::size_t section_file_size = static_cast<std::size_t>(
        std::min<std::uint64_t>(section.file_size, image.fileBytes().size() - section.file_offset));
    if (start_offset >= section_file_size)
    {
      continue;
    }

    const std::size_t length = static_cast<std::size_t>(
        std::min<std::uint64_t>(requested_end - requested_start, section_file_size - start_offset));
    const auto section_file_offset = checkedIntegralCast<std::size_t>(section.file_offset);
    const auto slice_start =
        section_file_offset.has_value() ? checkedAdd(*section_file_offset, start_offset) : std::nullopt;
    const auto slice_end = slice_start.has_value() ? checkedAdd(*slice_start, length) : std::nullopt;
    const auto begin_index =
        slice_start.has_value() ? checkedIntegralCast<std::ptrdiff_t>(*slice_start) : std::nullopt;
    const auto end_index =
        slice_end.has_value() ? checkedIntegralCast<std::ptrdiff_t>(*slice_end) : std::nullopt;
    if (!slice_end.has_value() || *slice_end > image.fileBytes().size() ||
        !begin_index.has_value() || !end_index.has_value())
    {
      return Result<DisassemblyResult>::failure(
          Error::disassembly("executable section slice exceeds host size limits"));
    }

    const auto begin = image.fileBytes().begin() + *begin_index;
    const auto end = image.fileBytes().begin() + *end_index;
    std::vector<std::uint8_t> bytes(begin, end);

    Result<DisassemblyResult> partial =
        disassembleBufferInternal(bytes, requested_start, section.name, &image);
    if (!partial)
    {
      return partial;
    }

    for (const Instruction& instruction : partial.value().instructions())
    {
      result.addInstruction(instruction);
    }
  }

  if (options.section_name.has_value() && result.instructions().empty())
  {
    const auto section = image.findSectionByName(*options.section_name);
    if (!section.has_value())
    {
      return Result<DisassemblyResult>::failure(
          Error::notFound("section not found: " + *options.section_name));
    }
    if (!section->executable)
    {
      return Result<DisassemblyResult>::failure(
          Error::invalidArgument("requested section is not executable: " + *options.section_name));
    }
  }

  return Result<DisassemblyResult>::success(std::move(result));
}

Result<DisassemblyResult> Disassembler::disassembleBytes(
    Address base_address,
    const std::vector<std::uint8_t>& bytes,
    std::string section_name) const
{
  return disassembleBufferInternal(bytes, base_address, section_name, nullptr);
}

}  // namespace binaryatlas
