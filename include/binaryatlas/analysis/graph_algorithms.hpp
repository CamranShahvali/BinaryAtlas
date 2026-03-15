#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "binaryatlas/core/types.hpp"

namespace binaryatlas
{

using AdjacencyList = std::unordered_map<Address, std::vector<Address>>;

struct StronglyConnectedComponents
{
  std::vector<std::vector<Address>> components;
};

[[nodiscard]] std::vector<Address> depthFirstReachable(
    const AdjacencyList& adjacency,
    Address start);

[[nodiscard]] StronglyConnectedComponents tarjanStronglyConnectedComponents(
    const AdjacencyList& adjacency);

[[nodiscard]] std::size_t connectedComponentCount(const AdjacencyList& adjacency);

}  // namespace binaryatlas
