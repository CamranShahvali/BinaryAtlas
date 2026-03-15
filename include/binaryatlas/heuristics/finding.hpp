#pragma once

#include <optional>
#include <string>
#include <vector>

#include "binaryatlas/core/types.hpp"

namespace binaryatlas
{

struct HeuristicFinding
{
  std::string category;
  std::optional<Address> address;
  std::optional<Address> function;
  double confidence {};
  std::vector<std::string> evidence;
  std::string explanation;
};

}  // namespace binaryatlas
