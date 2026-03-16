#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>

namespace binaryatlas
{

template <typename T>
concept Integral = std::is_integral_v<T>;

template <typename T>
concept UnsignedIntegral = Integral<T> && std::is_unsigned_v<T>;

template <UnsignedIntegral T> [[nodiscard]] constexpr bool addWouldOverflow(T left, T right)
{
  return left > std::numeric_limits<T>::max() - right;
}

template <UnsignedIntegral T> [[nodiscard]] constexpr bool multiplyWouldOverflow(T left, T right)
{
  return right != 0 && left > std::numeric_limits<T>::max() / right;
}

template <UnsignedIntegral T> [[nodiscard]] constexpr std::optional<T> checkedAdd(T left, T right)
{
  if (addWouldOverflow(left, right))
  {
    return std::nullopt;
  }
  return static_cast<T>(left + right);
}

template <UnsignedIntegral T>
[[nodiscard]] constexpr std::optional<T> checkedMultiply(T left, T right)
{
  if (multiplyWouldOverflow(left, right))
  {
    return std::nullopt;
  }
  return static_cast<T>(left * right);
}

template <Integral To, Integral From>
[[nodiscard]] constexpr std::optional<To> checkedIntegralCast(From value)
{
  if constexpr (std::is_signed_v<From>)
  {
    const auto signed_value = static_cast<std::intmax_t>(value);
    if constexpr (std::is_signed_v<To>)
    {
      if (signed_value < static_cast<std::intmax_t>(std::numeric_limits<To>::min()) ||
          signed_value > static_cast<std::intmax_t>(std::numeric_limits<To>::max()))
      {
        return std::nullopt;
      }
    }
    else
    {
      if (signed_value < 0)
      {
        return std::nullopt;
      }
      if (static_cast<std::uintmax_t>(signed_value) >
          static_cast<std::uintmax_t>(std::numeric_limits<To>::max()))
      {
        return std::nullopt;
      }
    }
  }
  else
  {
    const auto unsigned_value = static_cast<std::uintmax_t>(value);
    if (unsigned_value > static_cast<std::uintmax_t>(std::numeric_limits<To>::max()))
    {
      return std::nullopt;
    }
  }

  return static_cast<To>(value);
}

template <UnsignedIntegral T>
[[nodiscard]] constexpr bool rangeFitsWithin(T offset, T size, T bound)
{
  if (offset > bound)
  {
    return false;
  }

  const std::optional<T> end = checkedAdd(offset, size);
  return end.has_value() && *end <= bound;
}

template <UnsignedIntegral T>
[[nodiscard]] constexpr std::optional<T> checkedRangeEnd(T start, T size)
{
  return checkedAdd(start, size);
}

} // namespace binaryatlas
