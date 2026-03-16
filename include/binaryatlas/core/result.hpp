#pragma once

#include <optional>
#include <stdexcept>
#include <utility>

#include "binaryatlas/core/error.hpp"

namespace binaryatlas
{

class Status
{
public:
  static Status success();
  static Status failure(Error error);

  [[nodiscard]] bool ok() const;
  [[nodiscard]] explicit operator bool() const;
  [[nodiscard]] const Error& error() const;

private:
  explicit Status(std::optional<Error> error);

  std::optional<Error> error_;
};

template <typename T> class Result
{
public:
  static Result success(T value)
  {
    return Result(std::move(value), std::nullopt);
  }

  static Result failure(Error error)
  {
    return Result(std::nullopt, std::move(error));
  }

  [[nodiscard]] bool ok() const
  {
    return value_.has_value();
  }

  [[nodiscard]] explicit operator bool() const
  {
    return ok();
  }

  [[nodiscard]] const T& value() const&
  {
    if (!value_)
    {
      throw std::logic_error("Result does not contain a value");
    }
    return *value_;
  }

  [[nodiscard]] T& value() &
  {
    if (!value_)
    {
      throw std::logic_error("Result does not contain a value");
    }
    return *value_;
  }

  [[nodiscard]] T&& value() &&
  {
    if (!value_)
    {
      throw std::logic_error("Result does not contain a value");
    }
    return std::move(*value_);
  }

  [[nodiscard]] const Error& error() const
  {
    if (value_)
    {
      throw std::logic_error("Result does not contain an error");
    }
    return *error_;
  }

private:
  Result(std::optional<T> value, std::optional<Error> error)
      : value_(std::move(value)), error_(std::move(error))
  {
  }

  std::optional<T> value_;
  std::optional<Error> error_;
};

} // namespace binaryatlas
