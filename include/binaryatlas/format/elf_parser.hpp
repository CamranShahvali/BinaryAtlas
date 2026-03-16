#pragma once

#include "binaryatlas/format/binary_format.hpp"

namespace binaryatlas
{

class ElfParser final : public BinaryFormatParser
{
public:
  [[nodiscard]] Result<BinaryImage> parseFile(const std::filesystem::path& path) const override;
  [[nodiscard]] Result<BinaryImage>
  parseBuffer(std::string source_name, std::vector<std::uint8_t> file_bytes) const override;
};

} // namespace binaryatlas
