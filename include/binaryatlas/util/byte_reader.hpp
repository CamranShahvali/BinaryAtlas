#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

namespace binaryatlas
{

class ByteReader
{
public:
  explicit ByteReader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}
  explicit ByteReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

  [[nodiscard]] std::size_t size() const
  {
    return bytes_.size();
  }

  [[nodiscard]] bool canRead(std::size_t offset, std::size_t length) const
  {
    return offset <= bytes_.size() && length <= bytes_.size() - offset;
  }

  template <typename T>
  [[nodiscard]] std::optional<T> readObject(std::size_t offset) const
  {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    if (!canRead(offset, sizeof(T)))
    {
      return std::nullopt;
    }

    T value {};
    std::memcpy(&value, bytes_.data() + offset, sizeof(T));
    return value;
  }

  [[nodiscard]] std::optional<std::span<const std::uint8_t>> readSpan(
      std::size_t offset,
      std::size_t length) const
  {
    if (!canRead(offset, length))
    {
      return std::nullopt;
    }
    return bytes_.subspan(offset, length);
  }

private:
  std::span<const std::uint8_t> bytes_;
};

}  // namespace binaryatlas
