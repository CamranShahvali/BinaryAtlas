#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "binaryatlas/core/binary_image.hpp"
#include "binaryatlas/core/result.hpp"

namespace binaryatlas
{

class BinaryFormatParser
{
public:
  virtual ~BinaryFormatParser() = default;

  [[nodiscard]] virtual Result<BinaryImage> parseFile(const std::filesystem::path& path) const = 0;
  [[nodiscard]] virtual Result<BinaryImage> parseBuffer(
      std::string source_name,
      std::vector<std::uint8_t> file_bytes) const = 0;
};

class BinaryLoader
{
public:
  [[nodiscard]] static Result<BinaryImage> load(const std::filesystem::path& path);
};

}  // namespace binaryatlas
