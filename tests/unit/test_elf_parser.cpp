#include <algorithm>
#include <cstring>
#include <elf.h>
#include <limits>

#include <catch2/catch_test_macros.hpp>

#include "binaryatlas/format/elf_parser.hpp"
#include "binaryatlas/util/file_io.hpp"
#include "test_support.hpp"

namespace
{

using namespace binaryatlas;

TEST_CASE("ELF parser loads metadata, sections, and symbols from fixtures")
{
  const Result<BinaryImage> image = test_support::loadFixture("direct_calls");
  REQUIRE(image);

  CHECK(image.value().metadata().format == BinaryFormat::elf);
  CHECK(image.value().metadata().architecture == Architecture::x86_64);
  CHECK(image.value().metadata().entry_point != 0);

  const std::optional<Section> text = image.value().findSectionByName(".text");
  REQUIRE(text.has_value());
  CHECK(text->executable);

  const auto symbol =
      std::find_if(image.value().symbols().begin(), image.value().symbols().end(),
                   [](const Symbol& candidate) { return candidate.name == "caller"; });
  REQUIRE(symbol != image.value().symbols().end());
  CHECK(symbol->type == SymbolType::function);
}

TEST_CASE("ELF parser supports address translation for executable sections")
{
  const Result<BinaryImage> image = test_support::loadFixture("straight_line");
  REQUIRE(image);

  const std::optional<Section> text = image.value().findSectionByName(".text");
  REQUIRE(text.has_value());

  const std::optional<std::size_t> file_offset = image.value().vaToFileOffset(text->range.start);
  REQUIRE(file_offset.has_value());
  CHECK(*file_offset == text->file_offset);

  const std::optional<Address> address = image.value().fileOffsetToVa(text->file_offset);
  REQUIRE(address.has_value());
  CHECK(*address == text->range.start);
}

TEST_CASE("ELF parser rejects malformed and truncated inputs")
{
  ElfParser parser;

  const Result<BinaryImage> malformed = parser.parseBuffer("malformed", {0x7fU, 'E', 'L', 'X'});
  REQUIRE_FALSE(malformed);

  const Result<BinaryImage> truncated = parser.parseBuffer("truncated", {0x7fU, 'E', 'L', 'F'});
  REQUIRE_FALSE(truncated);
}

TEST_CASE("ELF parser rejects invalid section table locations")
{
  const Result<std::vector<std::uint8_t>> bytes =
      readBinaryFile(test_support::fixturePath("straight_line"));
  REQUIRE(bytes);
  REQUIRE(bytes.value().size() >= sizeof(Elf64_Ehdr));

  std::vector<std::uint8_t> corrupted = bytes.value();
  Elf64_Ehdr header{};
  std::memcpy(&header, corrupted.data(), sizeof(header));
  header.e_shoff = static_cast<std::uint64_t>(corrupted.size() - 8);
  header.e_shnum = 64;
  std::memcpy(corrupted.data(), &header, sizeof(header));

  ElfParser parser;
  const Result<BinaryImage> result = parser.parseBuffer("corrupted", std::move(corrupted));
  REQUIRE_FALSE(result);
}

TEST_CASE("ELF parser rejects overflowing section ranges")
{
  const Result<std::vector<std::uint8_t>> bytes =
      readBinaryFile(test_support::fixturePath("straight_line"));
  REQUIRE(bytes);
  REQUIRE(bytes.value().size() >= sizeof(Elf64_Ehdr));

  std::vector<std::uint8_t> corrupted = bytes.value();
  Elf64_Ehdr header{};
  std::memcpy(&header, corrupted.data(), sizeof(header));
  REQUIRE(header.e_shnum > 1);

  const std::size_t section_offset = static_cast<std::size_t>(header.e_shoff + header.e_shentsize);
  REQUIRE(section_offset + sizeof(Elf64_Shdr) <= corrupted.size());

  Elf64_Shdr section{};
  std::memcpy(&section, corrupted.data() + section_offset, sizeof(section));
  section.sh_addr = std::numeric_limits<std::uint64_t>::max() - 4;
  section.sh_size = 32;
  std::memcpy(corrupted.data() + section_offset, &section, sizeof(section));

  ElfParser parser;
  const Result<BinaryImage> result =
      parser.parseBuffer("overflowing-section", std::move(corrupted));
  REQUIRE_FALSE(result);
}

TEST_CASE("ELF parser rejects symbol tables that extend beyond file contents")
{
  const Result<std::vector<std::uint8_t>> bytes =
      readBinaryFile(test_support::fixturePath("direct_calls"));
  REQUIRE(bytes);
  REQUIRE(bytes.value().size() >= sizeof(Elf64_Ehdr));

  std::vector<std::uint8_t> corrupted = bytes.value();
  Elf64_Ehdr header{};
  std::memcpy(&header, corrupted.data(), sizeof(header));

  bool patched = false;
  for (std::size_t index = 0; index < header.e_shnum; ++index)
  {
    const std::size_t offset =
        static_cast<std::size_t>(header.e_shoff + (header.e_shentsize * index));
    REQUIRE(offset + sizeof(Elf64_Shdr) <= corrupted.size());

    Elf64_Shdr section{};
    std::memcpy(&section, corrupted.data() + offset, sizeof(section));
    if (section.sh_type != SHT_SYMTAB && section.sh_type != SHT_DYNSYM)
    {
      continue;
    }

    section.sh_offset = static_cast<std::uint64_t>(corrupted.size() - 8);
    section.sh_size = 64;
    std::memcpy(corrupted.data() + offset, &section, sizeof(section));
    patched = true;
    break;
  }

  REQUIRE(patched);

  ElfParser parser;
  const Result<BinaryImage> result =
      parser.parseBuffer("truncated-symbol-table", std::move(corrupted));
  REQUIRE_FALSE(result);
}

} // namespace
