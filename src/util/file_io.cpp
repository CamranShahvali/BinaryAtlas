#include "binaryatlas/util/file_io.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

#include "binaryatlas/core/error.hpp"

namespace binaryatlas
{

Status Status::success()
{
  return Status(std::nullopt);
}

Status Status::failure(Error error)
{
  return Status(std::move(error));
}

bool Status::ok() const
{
  return !error_.has_value();
}

Status::operator bool() const
{
  return ok();
}

const Error& Status::error() const
{
  if (!error_)
  {
    throw std::logic_error("Status does not contain an error");
  }
  return *error_;
}

Status::Status(std::optional<Error> error) : error_(std::move(error))
{
}

Result<std::vector<std::uint8_t>> readBinaryFile(const std::filesystem::path& path)
{
  std::ifstream stream(path, std::ios::binary);
  if (!stream)
  {
    return Result<std::vector<std::uint8_t>>::failure(
        Error::io("failed to open file: " + path.string()));
  }

  const std::vector<std::uint8_t> bytes {
      std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
  return Result<std::vector<std::uint8_t>>::success(bytes);
}

Status writeTextFile(const std::filesystem::path& path, std::string_view content)
{
  std::ofstream stream(path, std::ios::binary);
  if (!stream)
  {
    return Status::failure(Error::io("failed to open output file: " + path.string()));
  }

  stream << content;
  if (!stream.good())
  {
    return Status::failure(Error::io("failed to write output file: " + path.string()));
  }

  return Status::success();
}

Status ensureDirectory(const std::filesystem::path& path)
{
  std::error_code error;
  std::filesystem::create_directories(path, error);
  if (error)
  {
    return Status::failure(
        Error::io("failed to create directory " + path.string() + ": " + error.message()));
  }
  return Status::success();
}

}  // namespace binaryatlas
