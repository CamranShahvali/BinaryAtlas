#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace binaryatlas
{

[[nodiscard]] inline std::string toLowerCopy(std::string_view value)
{
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return result;
}

[[nodiscard]] inline bool icontains(std::string_view haystack, std::string_view needle)
{
  const std::string lowered_haystack = toLowerCopy(haystack);
  const std::string lowered_needle = toLowerCopy(needle);
  return lowered_haystack.find(lowered_needle) != std::string::npos;
}

[[nodiscard]] inline std::optional<std::string> firstMatchingKeyword(
    std::string_view value,
    const std::vector<std::string>& keywords)
{
  for (const std::string& keyword : keywords)
  {
    if (icontains(value, keyword))
    {
      return keyword;
    }
  }
  return std::nullopt;
}

[[nodiscard]] inline std::string join(
    const std::vector<std::string>& items,
    std::string_view separator)
{
  std::ostringstream stream;
  for (std::size_t index = 0; index < items.size(); ++index)
  {
    if (index > 0)
    {
      stream << separator;
    }
    stream << items[index];
  }
  return stream.str();
}

[[nodiscard]] inline std::string formatHex(std::uint64_t value)
{
  std::ostringstream stream;
  stream << "0x" << std::hex << std::nouppercase << value;
  return stream.str();
}

}  // namespace binaryatlas
