#pragma once

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

#include "binaryatlas/core/types.hpp"

namespace binaryatlas
{

class BinaryImage
{
public:
  BinaryImage(
      BinaryMetadata metadata,
      std::filesystem::path source_path,
      std::vector<std::uint8_t> file_bytes,
      std::vector<Section> sections,
      std::vector<Segment> segments,
      std::vector<Symbol> symbols,
      std::vector<Relocation> relocations,
      std::vector<std::string> imports,
      std::vector<std::string> exports,
      std::vector<ExtractedString> strings);

  [[nodiscard]] const BinaryMetadata& metadata() const;
  [[nodiscard]] const std::filesystem::path& sourcePath() const;
  [[nodiscard]] const std::vector<std::uint8_t>& fileBytes() const;
  [[nodiscard]] const std::vector<Section>& sections() const;
  [[nodiscard]] const std::vector<Segment>& segments() const;
  [[nodiscard]] const std::vector<Symbol>& symbols() const;
  [[nodiscard]] const std::vector<Relocation>& relocations() const;
  [[nodiscard]] const std::vector<std::string>& imports() const;
  [[nodiscard]] const std::vector<std::string>& exports() const;
  [[nodiscard]] const std::vector<ExtractedString>& extractedStrings() const;

  [[nodiscard]] std::vector<AddressRange> executableRanges() const;
  [[nodiscard]] bool isExecutableAddress(Address address) const;
  [[nodiscard]] std::optional<Section> findSectionByName(std::string_view name) const;
  [[nodiscard]] std::optional<Section> findSectionContaining(Address address) const;
  [[nodiscard]] std::optional<std::size_t> vaToFileOffset(Address address) const;
  [[nodiscard]] std::optional<Address> fileOffsetToVa(std::size_t offset) const;
  [[nodiscard]] std::vector<std::uint8_t> readBytes(Address address, std::size_t size) const;

private:
  BinaryMetadata metadata_;
  std::filesystem::path source_path_;
  std::vector<std::uint8_t> file_bytes_;
  std::vector<Section> sections_;
  std::vector<Segment> segments_;
  std::vector<Symbol> symbols_;
  std::vector<Relocation> relocations_;
  std::vector<std::string> imports_;
  std::vector<std::string> exports_;
  std::vector<ExtractedString> strings_;
};

}  // namespace binaryatlas
