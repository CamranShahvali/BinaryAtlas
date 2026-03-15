#pragma once

#include <optional>
#include <string>
#include <vector>

#include "binaryatlas/core/types.hpp"

namespace binaryatlas
{

struct BlockEdge
{
  Address source {};
  std::optional<Address> target;
  BlockEdgeType type {BlockEdgeType::fallthrough};
  bool intra_function {true};
};

struct CallSite
{
  Address instruction_address {};
  std::optional<Address> target;
  bool indirect {false};
  bool resolved {true};
  std::string operand_text;
};

struct BasicBlock
{
  Address start {};
  Address end {};
  std::vector<Address> instruction_addresses;
  std::vector<BlockEdge> outgoing_edges;
  bool partial {false};
};

}  // namespace binaryatlas
