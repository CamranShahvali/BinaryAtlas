#pragma once

#include <string>
#include <vector>

#include "binaryatlas/core/binary_image.hpp"
#include "binaryatlas/core/result.hpp"
#include "binaryatlas/disasm/instruction.hpp"

namespace binaryatlas
{

class Disassembler
{
public:
  [[nodiscard]] Result<DisassemblyResult> disassemble(
      const BinaryImage& image,
      const DisassemblyOptions& options = {}) const;

  [[nodiscard]] Result<DisassemblyResult> disassembleBytes(
      Address base_address,
      const std::vector<std::uint8_t>& bytes,
      std::string section_name = ".text") const;
};

}  // namespace binaryatlas
