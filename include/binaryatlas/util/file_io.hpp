#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "binaryatlas/core/result.hpp"

namespace binaryatlas
{

[[nodiscard]] Result<std::vector<std::uint8_t>> readBinaryFile(const std::filesystem::path& path);
[[nodiscard]] Status writeTextFile(const std::filesystem::path& path, std::string_view content);
[[nodiscard]] Status ensureDirectory(const std::filesystem::path& path);

} // namespace binaryatlas
