#include "binaryatlas/core/binary_image.hpp"

#include <algorithm>

namespace binaryatlas
{

BinaryImage::BinaryImage(
    BinaryMetadata metadata,
    std::filesystem::path source_path,
    std::vector<std::uint8_t> file_bytes,
    std::vector<Section> sections,
    std::vector<Segment> segments,
    std::vector<Symbol> symbols,
    std::vector<Relocation> relocations,
    std::vector<std::string> imports,
    std::vector<std::string> exports,
    std::vector<ExtractedString> strings)
    : metadata_(std::move(metadata)),
      source_path_(std::move(source_path)),
      file_bytes_(std::move(file_bytes)),
      sections_(std::move(sections)),
      segments_(std::move(segments)),
      symbols_(std::move(symbols)),
      relocations_(std::move(relocations)),
      imports_(std::move(imports)),
      exports_(std::move(exports)),
      strings_(std::move(strings))
{
}

const BinaryMetadata& BinaryImage::metadata() const
{
  return metadata_;
}

const std::filesystem::path& BinaryImage::sourcePath() const
{
  return source_path_;
}

const std::vector<std::uint8_t>& BinaryImage::fileBytes() const
{
  return file_bytes_;
}

const std::vector<Section>& BinaryImage::sections() const
{
  return sections_;
}

const std::vector<Segment>& BinaryImage::segments() const
{
  return segments_;
}

const std::vector<Symbol>& BinaryImage::symbols() const
{
  return symbols_;
}

const std::vector<Relocation>& BinaryImage::relocations() const
{
  return relocations_;
}

const std::vector<std::string>& BinaryImage::imports() const
{
  return imports_;
}

const std::vector<std::string>& BinaryImage::exports() const
{
  return exports_;
}

const std::vector<ExtractedString>& BinaryImage::extractedStrings() const
{
  return strings_;
}

std::vector<AddressRange> BinaryImage::executableRanges() const
{
  std::vector<AddressRange> ranges;
  for (const Section& section : sections_)
  {
    if (section.executable && section.range.size() > 0)
    {
      ranges.push_back(section.range);
    }
  }
  return ranges;
}

bool BinaryImage::isExecutableAddress(Address address) const
{
  return std::any_of(sections_.begin(), sections_.end(), [address](const Section& section) {
    return section.executable && section.range.contains(address);
  });
}

std::optional<Section> BinaryImage::findSectionByName(std::string_view name) const
{
  const auto iterator = std::find_if(sections_.begin(), sections_.end(), [name](const Section& section) {
    return section.name == name;
  });
  if (iterator == sections_.end())
  {
    return std::nullopt;
  }
  return *iterator;
}

std::optional<Section> BinaryImage::findSectionContaining(Address address) const
{
  const auto iterator =
      std::find_if(sections_.begin(), sections_.end(), [address](const Section& section) {
        return section.range.contains(address);
      });
  if (iterator == sections_.end())
  {
    return std::nullopt;
  }
  return *iterator;
}

std::optional<std::size_t> BinaryImage::vaToFileOffset(Address address) const
{
  for (const Section& section : sections_)
  {
    if (!section.range.contains(address))
    {
      continue;
    }

    const std::uint64_t delta = address - section.range.start;
    if (delta < section.file_size)
    {
      return static_cast<std::size_t>(section.file_offset + delta);
    }
  }

  for (const Segment& segment : segments_)
  {
    if (!segment.range.contains(address))
    {
      continue;
    }

    const std::uint64_t delta = address - segment.range.start;
    if (delta < segment.file_size)
    {
      return static_cast<std::size_t>(segment.file_offset + delta);
    }
  }

  return std::nullopt;
}

std::optional<Address> BinaryImage::fileOffsetToVa(std::size_t offset) const
{
  for (const Section& section : sections_)
  {
    if (offset < section.file_offset || offset >= section.file_offset + section.file_size)
    {
      continue;
    }

    return section.range.start + static_cast<Address>(offset - section.file_offset);
  }

  for (const Segment& segment : segments_)
  {
    if (offset < segment.file_offset || offset >= segment.file_offset + segment.file_size)
    {
      continue;
    }

    return segment.range.start + static_cast<Address>(offset - segment.file_offset);
  }

  return std::nullopt;
}

std::vector<std::uint8_t> BinaryImage::readBytes(Address address, std::size_t size) const
{
  const std::optional<std::size_t> file_offset = vaToFileOffset(address);
  if (!file_offset.has_value())
  {
    return {};
  }

  if (*file_offset > file_bytes_.size() || size > file_bytes_.size() - *file_offset)
  {
    return {};
  }

  return {
      file_bytes_.begin() + static_cast<std::ptrdiff_t>(*file_offset),
      file_bytes_.begin() + static_cast<std::ptrdiff_t>(*file_offset + size)};
}

}  // namespace binaryatlas
