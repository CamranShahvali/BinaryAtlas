#include "binaryatlas/core/error.hpp"

namespace binaryatlas
{

Error Error::invalidArgument(std::string message)
{
  return {ErrorCode::invalid_argument, std::move(message)};
}

Error Error::io(std::string message)
{
  return {ErrorCode::io_error, std::move(message)};
}

Error Error::parse(std::string message)
{
  return {ErrorCode::parse_error, std::move(message)};
}

Error Error::unsupported(std::string message)
{
  return {ErrorCode::unsupported_format, std::move(message)};
}

Error Error::disassembly(std::string message)
{
  return {ErrorCode::disassembly_error, std::move(message)};
}

Error Error::analysis(std::string message)
{
  return {ErrorCode::analysis_error, std::move(message)};
}

Error Error::notFound(std::string message)
{
  return {ErrorCode::not_found, std::move(message)};
}

std::string toString(ErrorCode code)
{
  switch (code)
  {
    case ErrorCode::invalid_argument:
      return "invalid_argument";
    case ErrorCode::io_error:
      return "io_error";
    case ErrorCode::parse_error:
      return "parse_error";
    case ErrorCode::unsupported_format:
      return "unsupported_format";
    case ErrorCode::disassembly_error:
      return "disassembly_error";
    case ErrorCode::analysis_error:
      return "analysis_error";
    case ErrorCode::not_found:
      return "not_found";
  }
  return "unknown";
}

}  // namespace binaryatlas
