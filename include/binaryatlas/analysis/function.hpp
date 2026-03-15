#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "binaryatlas/analysis/basic_block.hpp"

namespace binaryatlas
{

struct FunctionSeed
{
  Address address {};
  std::string source;
  std::string label;
  double confidence {};
};

struct FunctionMetrics
{
  std::size_t block_count {};
  std::size_t edge_count {};
  std::size_t instruction_count {};
  std::uint64_t size_bytes {};
  std::size_t loop_count {};
  std::size_t connected_components {};
  std::size_t max_in_degree {};
  std::size_t max_out_degree {};
  double average_out_degree {};
  std::size_t call_out_degree {};
  std::size_t cyclomatic_complexity {};
};

struct RecoveredFunction
{
  Address entry {};
  std::string name;
  std::vector<FunctionSeed> seed_reasons;
  std::vector<BasicBlock> blocks;
  std::vector<CallSite> call_sites;
  std::vector<Address> callees;
  std::vector<std::string> referenced_strings;
  FunctionMetrics metrics;
  bool partial {false};
  bool ambiguous {false};
};

struct CallGraphEdge
{
  Address source {};
  Address target {};
  bool external {false};
  std::string target_name;
};

}  // namespace binaryatlas
