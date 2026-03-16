#pragma once

#include <string>

namespace binaryatlas
{

enum class ErrorCode
{
  invalid_argument,
  io_error,
  parse_error,
  unsupported_format,
  disassembly_error,
  analysis_error,
  not_found
};

struct Error
{
  ErrorCode code{};
  std::string message;

  [[nodiscard]] static Error invalidArgument(std::string message);
  [[nodiscard]] static Error io(std::string message);
  [[nodiscard]] static Error parse(std::string message);
  [[nodiscard]] static Error unsupported(std::string message);
  [[nodiscard]] static Error disassembly(std::string message);
  [[nodiscard]] static Error analysis(std::string message);
  [[nodiscard]] static Error notFound(std::string message);
};

[[nodiscard]] std::string toString(ErrorCode code);

} // namespace binaryatlas
