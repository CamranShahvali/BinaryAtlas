#include "binaryatlas/format/elf_parser.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <elf.h>
#include <limits>
#include <set>
#include <span>
#include <string_view>

#include "binaryatlas/core/error.hpp"
#include "binaryatlas/util/arithmetic.hpp"
#include "binaryatlas/util/byte_reader.hpp"
#include "binaryatlas/util/file_io.hpp"

namespace binaryatlas
{
namespace
{

constexpr std::array<unsigned char, 4> kElfMagic {0x7fU, 'E', 'L', 'F'};

[[nodiscard]] bool hasPrefix(
    const std::vector<std::uint8_t>& bytes,
    std::span<const unsigned char> prefix)
{
  if (bytes.size() < prefix.size())
  {
    return false;
  }

  return std::equal(prefix.begin(), prefix.end(), bytes.begin(), [](unsigned char expected, std::uint8_t actual) {
    return expected == actual;
  });
}

[[nodiscard]] std::string readCString(std::span<const std::uint8_t> bytes, std::size_t offset)
{
  if (offset >= bytes.size())
  {
    return {};
  }

  std::string result;
  for (std::size_t index = offset; index < bytes.size() && bytes[index] != 0U; ++index)
  {
    result.push_back(static_cast<char>(bytes[index]));
  }
  return result;
}

[[nodiscard]] SymbolType mapSymbolType(unsigned char info)
{
  switch (ELF64_ST_TYPE(info))
  {
    case STT_NOTYPE:
      return SymbolType::no_type;
    case STT_FUNC:
      return SymbolType::function;
    case STT_OBJECT:
      return SymbolType::object;
    case STT_SECTION:
      return SymbolType::section;
    case STT_FILE:
      return SymbolType::file;
    case STT_TLS:
      return SymbolType::tls;
    default:
      return SymbolType::unknown;
  }
}

[[nodiscard]] SymbolBinding mapSymbolBinding(unsigned char info)
{
  switch (ELF64_ST_BIND(info))
  {
    case STB_LOCAL:
      return SymbolBinding::local;
    case STB_GLOBAL:
      return SymbolBinding::global;
    case STB_WEAK:
      return SymbolBinding::weak;
    default:
      return SymbolBinding::unknown;
  }
}

[[nodiscard]] std::string programHeaderTypeName(std::uint32_t type)
{
  switch (type)
  {
    case PT_NULL:
      return "PT_NULL";
    case PT_LOAD:
      return "PT_LOAD";
    case PT_DYNAMIC:
      return "PT_DYNAMIC";
    case PT_INTERP:
      return "PT_INTERP";
    case PT_NOTE:
      return "PT_NOTE";
    case PT_PHDR:
      return "PT_PHDR";
    case PT_TLS:
      return "PT_TLS";
    default:
      return "PT_OTHER";
  }
}

[[nodiscard]] bool isLikelyPrintable(std::uint8_t byte)
{
  return byte == 0U ||
         std::isprint(static_cast<unsigned char>(byte)) != 0 ||
         byte == static_cast<std::uint8_t>('\t');
}

[[nodiscard]] std::vector<ExtractedString> extractStrings(
    const std::vector<std::uint8_t>& file_bytes,
    const std::vector<Section>& sections)
{
  std::vector<ExtractedString> strings;

  for (const Section& section : sections)
  {
    if (!section.alloc || section.file_size == 0 || section.file_offset >= file_bytes.size())
    {
      continue;
    }

    const std::size_t file_offset = static_cast<std::size_t>(section.file_offset);
    const std::size_t size = static_cast<std::size_t>(
        std::min<std::uint64_t>(section.file_size, file_bytes.size() - file_offset));
    std::size_t cursor = 0;
    while (cursor < size)
    {
      if (!isLikelyPrintable(file_bytes[file_offset + cursor]) || file_bytes[file_offset + cursor] == 0U)
      {
        ++cursor;
        continue;
      }

      const std::size_t start = cursor;
      std::string value;
      while (cursor < size && file_bytes[file_offset + cursor] != 0U &&
             isLikelyPrintable(file_bytes[file_offset + cursor]))
      {
        value.push_back(static_cast<char>(file_bytes[file_offset + cursor]));
        ++cursor;
      }

      if (value.size() >= 4)
      {
        strings.push_back(
            {section.name, section.range.start + static_cast<Address>(start), std::move(value)});
      }

      while (cursor < size && file_bytes[file_offset + cursor] != 0U &&
             isLikelyPrintable(file_bytes[file_offset + cursor]))
      {
        ++cursor;
      }
    }
  }

  std::sort(strings.begin(), strings.end(), [](const ExtractedString& left, const ExtractedString& right) {
    return left.address < right.address;
  });
  return strings;
}

[[nodiscard]] Result<std::span<const std::uint8_t>> readBoundedSpan(
    const ByteReader& reader,
    std::uint64_t offset,
    std::uint64_t size,
    std::string_view label)
{
  const auto readable_bytes = checkedIntegralCast<std::uint64_t>(reader.size());
  if (!readable_bytes.has_value())
  {
    return Result<std::span<const std::uint8_t>>::failure(
        Error::parse("host size limit exceeded while parsing " + std::string(label)));
  }

  if (!rangeFitsWithin(offset, size, *readable_bytes))
  {
    return Result<std::span<const std::uint8_t>>::failure(
        Error::parse(std::string(label) + " extends beyond file contents"));
  }

  const auto safe_offset = checkedIntegralCast<std::size_t>(offset);
  const auto safe_size = checkedIntegralCast<std::size_t>(size);
  if (!safe_offset.has_value() || !safe_size.has_value())
  {
    return Result<std::span<const std::uint8_t>>::failure(
        Error::parse(std::string(label) + " exceeds host size limits"));
  }

  const auto span = reader.readSpan(*safe_offset, *safe_size);
  if (!span.has_value())
  {
    return Result<std::span<const std::uint8_t>>::failure(
        Error::parse(std::string(label) + " extends beyond file contents"));
  }

  return Result<std::span<const std::uint8_t>>::success(*span);
}

[[nodiscard]] Result<std::uint64_t> computeEntryCount(
    std::uint64_t byte_size,
    std::uint64_t entry_size,
    std::string_view label)
{
  if (entry_size == 0)
  {
    return Result<std::uint64_t>::success(0);
  }

  if (byte_size % entry_size != 0)
  {
    return Result<std::uint64_t>::failure(
        Error::parse("malformed " + std::string(label) + " size"));
  }

  return Result<std::uint64_t>::success(byte_size / entry_size);
}

template <typename EntryType>
[[nodiscard]] Result<std::vector<EntryType>> readTable(
    const ByteReader& reader,
    std::uint64_t offset,
    std::uint64_t entry_size,
    std::uint64_t count,
    std::string_view label)
{
  if (entry_size != sizeof(EntryType))
  {
    return Result<std::vector<EntryType>>::failure(
        Error::parse("unexpected " + std::string(label) + " entry size"));
  }

  if (count == 0)
  {
    return Result<std::vector<EntryType>>::success({});
  }

  const auto total_bytes = checkedMultiply(entry_size, count);
  if (!total_bytes.has_value())
  {
    return Result<std::vector<EntryType>>::failure(
        Error::parse("overflow while parsing " + std::string(label)));
  }

  const auto readable_bytes = checkedIntegralCast<std::uint64_t>(reader.size());
  if (!readable_bytes.has_value())
  {
    return Result<std::vector<EntryType>>::failure(
        Error::parse("host size limit exceeded while parsing " + std::string(label)));
  }

  if (!rangeFitsWithin(offset, *total_bytes, *readable_bytes))
  {
    return Result<std::vector<EntryType>>::failure(
        Error::parse("truncated " + std::string(label) + " table"));
  }

  const auto safe_count = checkedIntegralCast<std::size_t>(count);
  if (!safe_count.has_value())
  {
    return Result<std::vector<EntryType>>::failure(
        Error::parse(std::string(label) + " table exceeds host size limits"));
  }

  std::vector<EntryType> entries;
  entries.reserve(*safe_count);
  for (std::uint64_t index = 0; index < count; ++index)
  {
    const auto relative_offset = checkedMultiply(entry_size, index);
    const auto entry_offset = relative_offset.has_value() ? checkedAdd(offset, *relative_offset)
                                                          : std::nullopt;
    const auto safe_offset =
        entry_offset.has_value() ? checkedIntegralCast<std::size_t>(*entry_offset) : std::nullopt;
    if (!safe_offset.has_value())
    {
      return Result<std::vector<EntryType>>::failure(
          Error::parse("overflow while parsing " + std::string(label)));
    }

    const auto entry = reader.readObject<EntryType>(*safe_offset);
    if (!entry.has_value())
    {
      return Result<std::vector<EntryType>>::failure(
          Error::parse("truncated " + std::string(label) + " table"));
    }
    entries.push_back(*entry);
  }

  return Result<std::vector<EntryType>>::success(std::move(entries));
}

}  // namespace

Result<BinaryImage> BinaryLoader::load(const std::filesystem::path& path)
{
  ElfParser parser;
  return parser.parseFile(path);
}

Result<BinaryImage> ElfParser::parseFile(const std::filesystem::path& path) const
{
  Result<std::vector<std::uint8_t>> file_bytes = readBinaryFile(path);
  if (!file_bytes)
  {
    return Result<BinaryImage>::failure(file_bytes.error());
  }

  return parseBuffer(path.string(), std::move(file_bytes.value()));
}

Result<BinaryImage> ElfParser::parseBuffer(
    std::string source_name,
    std::vector<std::uint8_t> file_bytes) const
{
  if (!hasPrefix(file_bytes, kElfMagic))
  {
    return Result<BinaryImage>::failure(Error::unsupported("unsupported file magic"));
  }

  const ByteReader reader(file_bytes);
  const auto file_size = checkedIntegralCast<std::uint64_t>(file_bytes.size());
  if (!file_size.has_value())
  {
    return Result<BinaryImage>::failure(Error::parse("file exceeds supported host size limits"));
  }
  const auto header = reader.readObject<Elf64_Ehdr>(0);
  if (!header.has_value())
  {
    return Result<BinaryImage>::failure(Error::parse("truncated ELF header"));
  }

  if (header->e_ident[EI_CLASS] != ELFCLASS64)
  {
    return Result<BinaryImage>::failure(Error::unsupported("only ELF64 is supported"));
  }
  if (header->e_ident[EI_DATA] != ELFDATA2LSB)
  {
    return Result<BinaryImage>::failure(
        Error::unsupported("only little-endian ELF binaries are supported"));
  }
  if (header->e_machine != EM_X86_64)
  {
    return Result<BinaryImage>::failure(Error::unsupported("only x86_64 ELF binaries are supported"));
  }
  if (header->e_version != EV_CURRENT)
  {
    return Result<BinaryImage>::failure(Error::parse("unsupported ELF version"));
  }
  if (header->e_shnum == 0 || header->e_shnum == PN_XNUM)
  {
    return Result<BinaryImage>::failure(
        Error::unsupported("extended ELF section numbering is not supported in this version"));
  }
  if (header->e_shstrndx == SHN_XINDEX)
  {
    return Result<BinaryImage>::failure(
        Error::unsupported("extended ELF section name table index is not supported"));
  }

  Result<std::vector<Elf64_Shdr>> section_headers =
      readTable<Elf64_Shdr>(reader, header->e_shoff, header->e_shentsize, header->e_shnum, "section");
  if (!section_headers)
  {
    return Result<BinaryImage>::failure(section_headers.error());
  }

  if (header->e_shstrndx >= section_headers.value().size())
  {
    return Result<BinaryImage>::failure(Error::parse("invalid section string table index"));
  }

  const Elf64_Shdr& shstrtab = section_headers.value()[header->e_shstrndx];
  const Result<std::span<const std::uint8_t>> shstrtab_span =
      readBoundedSpan(reader, shstrtab.sh_offset, shstrtab.sh_size, "section string table");
  if (!shstrtab_span)
  {
    return Result<BinaryImage>::failure(shstrtab_span.error());
  }

  std::vector<Section> sections;
  sections.reserve(section_headers.value().size());
  for (std::size_t section_index = 0; section_index < section_headers.value().size(); ++section_index)
  {
    const Elf64_Shdr& section_header = section_headers.value()[section_index];
    Section section;
    section.name = readCString(shstrtab_span.value(), section_header.sh_name);
    const std::string section_label =
        section.name.empty() ? "section[" + std::to_string(section_index) + "]" : section.name;

    const auto section_end = checkedRangeEnd(section_header.sh_addr, section_header.sh_size);
    if (!section_end.has_value())
    {
      return Result<BinaryImage>::failure(
          Error::parse("section range overflows address space: " + section_label));
    }
    if (section_header.sh_type != SHT_NOBITS && section_header.sh_size > 0 &&
        !rangeFitsWithin(section_header.sh_offset, section_header.sh_size, *file_size))
    {
      return Result<BinaryImage>::failure(
          Error::parse("section extends beyond file contents: " + section_label));
    }

    section.range.start = section_header.sh_addr;
    section.range.end = *section_end;
    section.file_offset = section_header.sh_offset;
    section.file_size = section_header.sh_size;
    section.alignment = section_header.sh_addralign;
    section.flags = section_header.sh_flags;
    section.type = section_header.sh_type;
    section.executable = (section_header.sh_flags & SHF_EXECINSTR) != 0U;
    section.writable = (section_header.sh_flags & SHF_WRITE) != 0U;
    section.alloc = (section_header.sh_flags & SHF_ALLOC) != 0U;
    sections.push_back(std::move(section));
  }

  Result<std::vector<Elf64_Phdr>> program_headers =
      readTable<Elf64_Phdr>(reader, header->e_phoff, header->e_phentsize, header->e_phnum, "program header");
  if (!program_headers)
  {
    return Result<BinaryImage>::failure(program_headers.error());
  }

  std::vector<Segment> segments;
  segments.reserve(program_headers.value().size());
  for (std::size_t segment_index = 0; segment_index < program_headers.value().size(); ++segment_index)
  {
    const Elf64_Phdr& program_header = program_headers.value()[segment_index];
    const auto segment_end = checkedRangeEnd(program_header.p_vaddr, program_header.p_memsz);
    if (!segment_end.has_value())
    {
      return Result<BinaryImage>::failure(
          Error::parse("segment range overflows address space: segment[" + std::to_string(segment_index) + "]"));
    }
    if (program_header.p_filesz > 0 &&
        !rangeFitsWithin(program_header.p_offset, program_header.p_filesz, *file_size))
    {
      return Result<BinaryImage>::failure(
          Error::parse("segment extends beyond file contents: segment[" + std::to_string(segment_index) + "]"));
    }

    Segment segment;
    segment.type_name = programHeaderTypeName(program_header.p_type);
    segment.range.start = program_header.p_vaddr;
    segment.range.end = *segment_end;
    segment.file_offset = program_header.p_offset;
    segment.file_size = program_header.p_filesz;
    segment.memory_size = program_header.p_memsz;
    segment.flags = program_header.p_flags;
    segment.type = program_header.p_type;
    segment.executable = (program_header.p_flags & PF_X) != 0U;
    segment.writable = (program_header.p_flags & PF_W) != 0U;
    segment.readable = (program_header.p_flags & PF_R) != 0U;
    segments.push_back(std::move(segment));
  }

  std::vector<Symbol> symbols;
  std::set<std::string> imports;
  std::set<std::string> exports;
  bool has_symtab = false;

  for (std::size_t section_index = 0; section_index < section_headers.value().size(); ++section_index)
  {
    const Elf64_Shdr& symtab_section = section_headers.value()[section_index];
    if (symtab_section.sh_type != SHT_SYMTAB && symtab_section.sh_type != SHT_DYNSYM)
    {
      continue;
    }

    if (symtab_section.sh_link >= section_headers.value().size())
    {
      return Result<BinaryImage>::failure(Error::parse("symbol table string link is out of range"));
    }

    has_symtab = has_symtab || symtab_section.sh_type == SHT_SYMTAB;
    const Elf64_Shdr& string_table_section = section_headers.value()[symtab_section.sh_link];
    const Result<std::span<const std::uint8_t>> string_table = readBoundedSpan(
        reader,
        string_table_section.sh_offset,
        string_table_section.sh_size,
        "ELF symbol string table");
    if (!string_table)
    {
      return Result<BinaryImage>::failure(string_table.error());
    }

    const Result<std::uint64_t> symbol_count =
        computeEntryCount(symtab_section.sh_size, symtab_section.sh_entsize, "symbol table");
    if (!symbol_count)
    {
      return Result<BinaryImage>::failure(symbol_count.error());
    }
    Result<std::vector<Elf64_Sym>> symbol_entries = readTable<Elf64_Sym>(
        reader,
        symtab_section.sh_offset,
        symtab_section.sh_entsize,
        symbol_count.value(),
        "symbol");
    if (!symbol_entries)
    {
      return Result<BinaryImage>::failure(symbol_entries.error());
    }

    const std::string table_name = sections[section_index].name;
    for (const Elf64_Sym& symbol_entry : symbol_entries.value())
    {
      Symbol symbol;
      symbol.name = readCString(string_table.value(), symbol_entry.st_name);
      symbol.address = symbol_entry.st_value;
      symbol.size = symbol_entry.st_size;
      symbol.type = mapSymbolType(symbol_entry.st_info);
      symbol.binding = mapSymbolBinding(symbol_entry.st_info);
      symbol.defined = symbol_entry.st_shndx != SHN_UNDEF;
      symbol.imported = symtab_section.sh_type == SHT_DYNSYM && !symbol.defined && !symbol.name.empty();
      symbol.exported = symtab_section.sh_type == SHT_DYNSYM && symbol.defined &&
                        (symbol.binding == SymbolBinding::global || symbol.binding == SymbolBinding::weak);
      symbol.source_table = table_name;
      if (symbol.defined && symbol_entry.st_shndx < sections.size())
      {
        symbol.section_name = sections[symbol_entry.st_shndx].name;
      }

      if (symbol.imported)
      {
        imports.insert(symbol.name);
      }
      if (symbol.exported)
      {
        exports.insert(symbol.name);
      }

      symbols.push_back(std::move(symbol));
    }
  }

  std::sort(symbols.begin(), symbols.end(), [](const Symbol& left, const Symbol& right) {
    if (left.address != right.address)
    {
      return left.address < right.address;
    }
    if (left.name != right.name)
    {
      return left.name < right.name;
    }
    return left.source_table < right.source_table;
  });

  std::vector<Relocation> relocations;
  for (std::size_t section_index = 0; section_index < section_headers.value().size(); ++section_index)
  {
    const Elf64_Shdr& relocation_section = section_headers.value()[section_index];
    if (relocation_section.sh_type != SHT_RELA && relocation_section.sh_type != SHT_REL)
    {
      continue;
    }

    if (relocation_section.sh_link >= section_headers.value().size())
    {
      return Result<BinaryImage>::failure(Error::parse("relocation symbol table link is out of range"));
    }

    const Elf64_Shdr& linked_symbol_section = section_headers.value()[relocation_section.sh_link];
    if (linked_symbol_section.sh_link >= section_headers.value().size())
    {
      return Result<BinaryImage>::failure(Error::parse("relocation string table link is out of range"));
    }

    const Elf64_Shdr& relocation_strings_section = section_headers.value()[linked_symbol_section.sh_link];
    const Result<std::span<const std::uint8_t>> relocation_strings = readBoundedSpan(
        reader,
        relocation_strings_section.sh_offset,
        relocation_strings_section.sh_size,
        "relocation string table");
    if (!relocation_strings)
    {
      return Result<BinaryImage>::failure(relocation_strings.error());
    }

    const Result<std::uint64_t> relocation_count = computeEntryCount(
        relocation_section.sh_size,
        relocation_section.sh_entsize,
        relocation_section.sh_type == SHT_RELA ? "rela relocation table" : "rel relocation table");
    if (!relocation_count)
    {
      return Result<BinaryImage>::failure(relocation_count.error());
    }

    if (relocation_section.sh_type == SHT_RELA)
    {
      Result<std::vector<Elf64_Rela>> entries = readTable<Elf64_Rela>(
          reader,
          relocation_section.sh_offset,
          relocation_section.sh_entsize,
          relocation_count.value(),
          "rela relocation");
      if (!entries)
      {
        return Result<BinaryImage>::failure(entries.error());
      }

      const Result<std::uint64_t> symbol_count = computeEntryCount(
          linked_symbol_section.sh_size,
          linked_symbol_section.sh_entsize,
          "relocation symbol table");
      if (!symbol_count)
      {
        return Result<BinaryImage>::failure(symbol_count.error());
      }
      Result<std::vector<Elf64_Sym>> relocation_symbols = readTable<Elf64_Sym>(
          reader,
          linked_symbol_section.sh_offset,
          linked_symbol_section.sh_entsize,
          symbol_count.value(),
          "relocation symbol");
      if (!relocation_symbols)
      {
        return Result<BinaryImage>::failure(relocation_symbols.error());
      }

      for (const Elf64_Rela& entry : entries.value())
      {
        const std::size_t symbol_index = static_cast<std::size_t>(ELF64_R_SYM(entry.r_info));
        std::string symbol_name;
        if (symbol_index < relocation_symbols.value().size())
        {
          symbol_name = readCString(
              relocation_strings.value(),
              relocation_symbols.value()[symbol_index].st_name);
        }

        relocations.push_back(
            {sections[section_index].name,
             std::move(symbol_name),
             entry.r_offset,
             static_cast<std::uint32_t>(ELF64_R_TYPE(entry.r_info)),
             entry.r_addend,
             true});
      }
    }
    else
    {
      Result<std::vector<Elf64_Rel>> entries = readTable<Elf64_Rel>(
          reader,
          relocation_section.sh_offset,
          relocation_section.sh_entsize,
          relocation_count.value(),
          "rel relocation");
      if (!entries)
      {
        return Result<BinaryImage>::failure(entries.error());
      }

      const Result<std::uint64_t> symbol_count = computeEntryCount(
          linked_symbol_section.sh_size,
          linked_symbol_section.sh_entsize,
          "relocation symbol table");
      if (!symbol_count)
      {
        return Result<BinaryImage>::failure(symbol_count.error());
      }
      Result<std::vector<Elf64_Sym>> relocation_symbols = readTable<Elf64_Sym>(
          reader,
          linked_symbol_section.sh_offset,
          linked_symbol_section.sh_entsize,
          symbol_count.value(),
          "relocation symbol");
      if (!relocation_symbols)
      {
        return Result<BinaryImage>::failure(relocation_symbols.error());
      }

      for (const Elf64_Rel& entry : entries.value())
      {
        const std::size_t symbol_index = static_cast<std::size_t>(ELF64_R_SYM(entry.r_info));
        std::string symbol_name;
        if (symbol_index < relocation_symbols.value().size())
        {
          symbol_name = readCString(
              relocation_strings.value(),
              relocation_symbols.value()[symbol_index].st_name);
        }

        relocations.push_back(
            {sections[section_index].name,
             std::move(symbol_name),
             entry.r_offset,
             static_cast<std::uint32_t>(ELF64_R_TYPE(entry.r_info)),
             0,
             false});
      }
    }
  }

  std::sort(relocations.begin(), relocations.end(), [](const Relocation& left, const Relocation& right) {
    if (left.address != right.address)
    {
      return left.address < right.address;
    }
    if (left.section_name != right.section_name)
    {
      return left.section_name < right.section_name;
    }
    return left.symbol_name < right.symbol_name;
  });

  BinaryMetadata metadata;
  metadata.format = BinaryFormat::elf;
  metadata.architecture = Architecture::x86_64;
  metadata.endianness = Endianness::little;
  metadata.format_version = "ELF64";
  metadata.source_path = source_name;
  metadata.file_size = file_bytes.size();
  metadata.entry_point = header->e_entry;
  metadata.abi = header->e_ident[EI_OSABI];
  metadata.abi_version = header->e_ident[EI_ABIVERSION];
  metadata.type = header->e_type;
  metadata.machine = header->e_machine;
  metadata.stripped = !has_symtab;

  std::vector<std::string> import_list(imports.begin(), imports.end());
  std::vector<std::string> export_list(exports.begin(), exports.end());
  std::vector<ExtractedString> strings = extractStrings(file_bytes, sections);

  return Result<BinaryImage>::success(BinaryImage(
      std::move(metadata),
      source_name,
      std::move(file_bytes),
      std::move(sections),
      std::move(segments),
      std::move(symbols),
      std::move(relocations),
      std::move(import_list),
      std::move(export_list),
      std::move(strings)));
}

}  // namespace binaryatlas
