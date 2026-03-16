#include <catch2/catch_test_macros.hpp>

#include "binaryatlas/disasm/disassembler.hpp"
#include "test_support.hpp"

namespace
{

using namespace binaryatlas;

TEST_CASE("Disassembler classifies direct and indirect control flow")
{
  Disassembler disassembler;
  const std::vector<std::uint8_t> bytes{0xe8, 0x05, 0x00, 0x00, 0x00, // call 0x100a
                                        0xff, 0x10,                   // call [rax]
                                        0x75, 0x02,                   // jne 0x100b
                                        0xeb, 0x00,                   // jmp 0x100b
                                        0xc3};                        // ret

  const Result<DisassemblyResult> result = disassembler.disassembleBytes(0x1000, bytes);
  REQUIRE(result);
  REQUIRE(result.value().instructions().size() >= 5);

  CHECK(result.value().instructions()[0].flow_kind == InstructionFlowKind::call_direct);
  CHECK(result.value().instructions()[0].branch_target == 0x100a);
  CHECK(result.value().instructions()[1].flow_kind == InstructionFlowKind::call_indirect);
  CHECK(result.value().instructions()[2].flow_kind == InstructionFlowKind::jump_conditional);
  CHECK(result.value().instructions()[3].flow_kind == InstructionFlowKind::jump_unconditional);
  CHECK(result.value().instructions().back().flow_kind == InstructionFlowKind::return_flow);
}

TEST_CASE("Disassembler emits invalid-instruction placeholders instead of failing")
{
  Disassembler disassembler;
  const std::vector<std::uint8_t> bytes{0x90, 0x0f};

  const Result<DisassemblyResult> result = disassembler.disassembleBytes(0x2000, bytes);
  REQUIRE(result);
  REQUIRE(result.value().instructions().size() == 2);
  CHECK(result.value().invalidInstructionCount() == 1);
  CHECK(result.value().instructions()[1].invalid);
}

TEST_CASE("Disassembler rejects non executable sections")
{
  const Result<BinaryImage> image = test_support::loadFixture("graphics_keywords");
  REQUIRE(image);

  Disassembler disassembler;
  DisassemblyOptions options;
  options.section_name = ".rodata";

  const Result<DisassemblyResult> result = disassembler.disassemble(image.value(), options);
  REQUIRE_FALSE(result);
}

} // namespace
