#include "binaryatlas/analysis/recovery.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>

#include "binaryatlas/analysis/metrics.hpp"
#include "binaryatlas/util/arithmetic.hpp"
#include "binaryatlas/util/string.hpp"

namespace binaryatlas
{
namespace
{

struct SeedRecord
{
  FunctionSeed seed;
  int priority {};
  bool hard_boundary {false};
};

struct RecoveredState
{
  RecoveredFunction function;
  std::set<Address> instructions;
};

[[nodiscard]] bool isFunctionSymbol(const Symbol& symbol)
{
  return symbol.defined && symbol.type == SymbolType::function && symbol.address != 0;
}

[[nodiscard]] std::string generatedFunctionName(Address address)
{
  return "sub_" + formatHex(address).substr(2);
}

[[nodiscard]] const Instruction* nextInstruction(const DisassemblyResult& disassembly, Address address)
{
  const auto iterator = disassembly.index().upper_bound(address);
  if (iterator == disassembly.index().end())
  {
    return nullptr;
  }
  return disassembly.find(iterator->first);
}

[[nodiscard]] const Instruction* previousInstruction(const DisassemblyResult& disassembly, Address address)
{
  const auto iterator = disassembly.index().lower_bound(address);
  if (iterator == disassembly.index().begin() || iterator == disassembly.index().end())
  {
    return nullptr;
  }
  const auto previous = std::prev(iterator);
  return disassembly.find(previous->first);
}

[[nodiscard]] bool isBoundaryLike(const Instruction* instruction)
{
  if (instruction == nullptr)
  {
    return true;
  }

  return instruction->flow_kind == InstructionFlowKind::return_flow ||
         instruction->flow_kind == InstructionFlowKind::trap ||
         instruction->flow_kind == InstructionFlowKind::invalid ||
         instruction->mnemonic == "int3" ||
         instruction->mnemonic == "nop";
}

[[nodiscard]] bool matchesFramePrologue(
    const Instruction* first,
    const Instruction* second,
    const Instruction* third)
{
  if (first == nullptr || second == nullptr)
  {
    return false;
  }

  if (first->mnemonic == "push" && first->operand_text == "rbp" && second->mnemonic == "mov" &&
      second->operand_text == "rbp, rsp")
  {
    return true;
  }

  return first->mnemonic == "endbr64" && second->mnemonic == "push" && second->operand_text == "rbp" &&
         third != nullptr && third->mnemonic == "mov" && third->operand_text == "rbp, rsp";
}

[[nodiscard]] std::vector<SeedRecord> collectSeeds(
    const BinaryImage& image,
    const DisassemblyResult& disassembly,
    bool enable_prologue_heuristics)
{
  std::map<Address, std::vector<SeedRecord>> by_address;

  auto add_seed = [&](Address address, std::string source, std::string label, double confidence, int priority, bool hard_boundary) {
    if (!image.isExecutableAddress(address) || disassembly.find(address) == nullptr)
    {
      return;
    }
    by_address[address].push_back({{address, std::move(source), std::move(label), confidence}, priority, hard_boundary});
  };

  add_seed(image.metadata().entry_point, "entry_point", "_start", 1.0, 0, true);

  for (const Symbol& symbol : image.symbols())
  {
    if (isFunctionSymbol(symbol))
    {
      add_seed(symbol.address, "symbol", symbol.name, 0.98, 1, true);
    }
  }

  for (const Instruction& instruction : disassembly.instructions())
  {
    if (instruction.flow_kind == InstructionFlowKind::call_direct && instruction.branch_target.has_value())
    {
      add_seed(*instruction.branch_target, "call_target", generatedFunctionName(*instruction.branch_target), 0.7, 2, false);
    }
  }

  if (enable_prologue_heuristics)
  {
    for (const Instruction& instruction : disassembly.instructions())
    {
      const Instruction* second = nextInstruction(disassembly, instruction.address);
      const Instruction* third = second == nullptr ? nullptr : nextInstruction(disassembly, second->address);
      if (!matchesFramePrologue(&instruction, second, third))
      {
        continue;
      }

      const Instruction* prev = previousInstruction(disassembly, instruction.address);
      if (!isBoundaryLike(prev))
      {
        continue;
      }

      add_seed(
          instruction.address,
          "prologue",
          generatedFunctionName(instruction.address),
          0.55,
          3,
          false);
    }
  }

  std::vector<SeedRecord> seeds;
  for (auto& [address, records] : by_address)
  {
    std::sort(records.begin(), records.end(), [](const SeedRecord& left, const SeedRecord& right) {
      if (left.priority != right.priority)
      {
        return left.priority < right.priority;
      }
      return left.seed.label < right.seed.label;
    });
    seeds.push_back(records.front());
  }

  std::sort(seeds.begin(), seeds.end(), [](const SeedRecord& left, const SeedRecord& right) {
    if (left.priority != right.priority)
    {
      return left.priority < right.priority;
    }
    return left.seed.address < right.seed.address;
  });
  return seeds;
}

[[nodiscard]] std::map<Address, std::vector<FunctionSeed>> seedReasonMap(const std::vector<SeedRecord>& seeds)
{
  std::map<Address, std::vector<FunctionSeed>> reasons;
  for (const SeedRecord& seed : seeds)
  {
    reasons[seed.seed.address].push_back(seed.seed);
  }
  return reasons;
}

[[nodiscard]] std::map<Address, std::string> functionSymbolNames(const BinaryImage& image)
{
  std::map<Address, std::string> names;
  for (const Symbol& symbol : image.symbols())
  {
    if (isFunctionSymbol(symbol) && !symbol.name.empty() && !names.contains(symbol.address))
    {
      names.emplace(symbol.address, symbol.name);
    }
  }
  return names;
}

[[nodiscard]] std::string resolveFunctionName(
    Address entry,
    const std::map<Address, std::string>& symbol_names,
    const BinaryMetadata& metadata)
{
  const auto iterator = symbol_names.find(entry);
  if (iterator != symbol_names.end())
  {
    return iterator->second;
  }
  if (entry == metadata.entry_point)
  {
    return "_start";
  }
  return generatedFunctionName(entry);
}

[[nodiscard]] std::vector<std::string> collectReferencedStrings(
    const RecoveredFunction& function,
    const DisassemblyResult& disassembly,
    const BinaryImage& image)
{
  std::set<std::string> strings;
  for (const BasicBlock& block : function.blocks)
  {
    for (Address address : block.instruction_addresses)
    {
      const Instruction* instruction = disassembly.find(address);
      if (instruction == nullptr)
      {
        continue;
      }

      for (const MemoryReference& reference : instruction->memory_references)
      {
        for (const ExtractedString& candidate : image.extractedStrings())
        {
          const auto string_size_bytes =
              checkedAdd(candidate.value.size(), std::size_t {1});
          const auto string_size = string_size_bytes.has_value()
                                       ? checkedIntegralCast<Address>(*string_size_bytes)
                                       : std::nullopt;
          const auto end = string_size.has_value() ? checkedAdd(candidate.address, *string_size)
                                                   : std::nullopt;
          if (!end.has_value())
          {
            continue;
          }
          if (reference.target >= candidate.address && reference.target < *end)
          {
            strings.insert(candidate.value);
          }
        }
      }
    }
  }

  return {strings.begin(), strings.end()};
}

[[nodiscard]] RecoveredState recoverFunction(
    Address entry,
    const std::vector<FunctionSeed>& reasons,
    const BinaryImage& image,
    const DisassemblyResult& disassembly,
    const std::set<Address>& hard_boundary_seeds,
    const std::map<Address, std::string>& symbol_names)
{
  RecoveredState state;
  state.function.entry = entry;
  state.function.name = resolveFunctionName(entry, symbol_names, image.metadata());
  state.function.seed_reasons = reasons;

  std::set<Address> pending {entry};
  std::set<Address> block_starts {entry};
  std::map<Address, BasicBlock> blocks;
  std::set<Address> callees;

  while (!pending.empty())
  {
    const Address start = *pending.begin();
    pending.erase(pending.begin());
    if (blocks.contains(start))
    {
      continue;
    }

    const Instruction* current = disassembly.find(start);
    if (current == nullptr)
    {
      state.function.partial = true;
      continue;
    }

    BasicBlock block;
    block.start = start;

    while (current != nullptr)
    {
      if (!block.instruction_addresses.empty() && block_starts.contains(current->address))
      {
        block.end = current->address;
        block.outgoing_edges.push_back({block.start, current->address, BlockEdgeType::fallthrough, true});
        pending.insert(current->address);
        break;
      }

      block.instruction_addresses.push_back(current->address);
      state.instructions.insert(current->address);
      const auto next_address_value =
          checkedAdd(current->address, static_cast<Address>(current->size));
      if (!next_address_value.has_value())
      {
        block.end = current->address;
        block.partial = true;
        state.function.partial = true;
        current = nullptr;
        break;
      }
      const Address next_address = *next_address_value;

      switch (current->flow_kind)
      {
        case InstructionFlowKind::invalid:
          block.end = next_address;
          block.partial = true;
          state.function.partial = true;
          current = nullptr;
          break;
        case InstructionFlowKind::normal:
        {
          const Instruction* next = disassembly.find(next_address);
          if (next == nullptr)
          {
            block.end = next_address;
            current = nullptr;
          }
          else
          {
            current = next;
          }
          break;
        }
        case InstructionFlowKind::call_direct:
        case InstructionFlowKind::call_indirect:
        {
          CallSite site;
          site.instruction_address = current->address;
          site.target = current->branch_target;
          site.indirect = current->flow_kind == InstructionFlowKind::call_indirect;
          site.resolved = site.target.has_value();
          site.operand_text = current->operand_text;
          state.function.call_sites.push_back(std::move(site));
          if (current->branch_target.has_value() && image.isExecutableAddress(*current->branch_target))
          {
            callees.insert(*current->branch_target);
          }

          block.end = next_address;
          const Instruction* next = disassembly.find(next_address);
          if (next != nullptr)
          {
            block_starts.insert(next_address);
            pending.insert(next_address);
            block.outgoing_edges.push_back({block.start, next_address, BlockEdgeType::call_fallthrough, true});
          }
          else
          {
            state.function.partial = true;
          }
          current = nullptr;
          break;
        }
        case InstructionFlowKind::jump_conditional:
        {
          block.end = next_address;
          if (current->branch_target.has_value() && disassembly.find(*current->branch_target) != nullptr &&
              image.isExecutableAddress(*current->branch_target))
          {
            block_starts.insert(*current->branch_target);
            pending.insert(*current->branch_target);
            block.outgoing_edges.push_back(
                {block.start, *current->branch_target, BlockEdgeType::true_branch, true});
          }
          else
          {
            block.outgoing_edges.push_back(
                {block.start, std::nullopt, BlockEdgeType::unresolved_indirect, false});
            state.function.partial = true;
          }

          const Instruction* next = disassembly.find(next_address);
          if (next != nullptr)
          {
            block_starts.insert(next_address);
            pending.insert(next_address);
            block.outgoing_edges.push_back({block.start, next_address, BlockEdgeType::false_branch, true});
          }
          else
          {
            state.function.partial = true;
          }
          current = nullptr;
          break;
        }
        case InstructionFlowKind::jump_unconditional:
        {
          block.end = next_address;
          if (current->branch_target.has_value() && disassembly.find(*current->branch_target) != nullptr &&
              image.isExecutableAddress(*current->branch_target))
          {
            const bool boundary_jump = *current->branch_target != entry &&
                                       hard_boundary_seeds.contains(*current->branch_target);
            block.outgoing_edges.push_back(
                {block.start,
                 *current->branch_target,
                 BlockEdgeType::jump,
                 !boundary_jump});
            if (!boundary_jump)
            {
              block_starts.insert(*current->branch_target);
              pending.insert(*current->branch_target);
            }
          }
          else
          {
            block.outgoing_edges.push_back(
                {block.start, std::nullopt, BlockEdgeType::unresolved_indirect, false});
            state.function.partial = true;
          }
          current = nullptr;
          break;
        }
        case InstructionFlowKind::jump_indirect:
          block.end = next_address;
          block.outgoing_edges.push_back(
              {block.start, std::nullopt, BlockEdgeType::unresolved_indirect, false});
          state.function.partial = true;
          current = nullptr;
          break;
        case InstructionFlowKind::return_flow:
        case InstructionFlowKind::trap:
          block.end = next_address;
          block.outgoing_edges.push_back({block.start, std::nullopt, BlockEdgeType::return_edge, false});
          current = nullptr;
          break;
      }
    }

    if (block.end == 0)
    {
      block.end = block.start;
      block.partial = true;
      state.function.partial = true;
    }

    blocks.emplace(block.start, std::move(block));
  }

  for (auto& [address, block] : blocks)
  {
    state.function.blocks.push_back(std::move(block));
  }
  std::sort(
      state.function.blocks.begin(),
      state.function.blocks.end(),
      [](const BasicBlock& left, const BasicBlock& right) { return left.start < right.start; });

  state.function.callees.assign(callees.begin(), callees.end());
  std::sort(
      state.function.call_sites.begin(),
      state.function.call_sites.end(),
      [](const CallSite& left, const CallSite& right) {
        return left.instruction_address < right.instruction_address;
      });
  state.function.referenced_strings = collectReferencedStrings(state.function, disassembly, image);
  state.function.metrics = computeFunctionMetrics(state.function);
  return state;
}

[[nodiscard]] std::vector<CallGraphEdge> buildCallGraph(
    const BinaryImage& image,
    const std::vector<RecoveredFunction>& functions)
{
  std::map<Address, const RecoveredFunction*> by_entry;
  for (const RecoveredFunction& function : functions)
  {
    by_entry.emplace(function.entry, &function);
  }

  std::vector<CallGraphEdge> edges;
  std::set<std::tuple<Address, Address, bool, std::string>> unique_edges;

  for (const RecoveredFunction& function : functions)
  {
    for (const CallSite& call_site : function.call_sites)
    {
      if (!call_site.target.has_value())
      {
        continue;
      }

      const auto iterator = by_entry.find(*call_site.target);
      if (iterator != by_entry.end())
      {
        if (unique_edges.emplace(function.entry, *call_site.target, false, iterator->second->name).second)
        {
          edges.push_back({function.entry, *call_site.target, false, iterator->second->name});
        }
        continue;
      }

      const auto symbol_iterator =
          std::find_if(image.symbols().begin(), image.symbols().end(), [&call_site](const Symbol& symbol) {
            return symbol.address == *call_site.target && !symbol.name.empty();
          });
      if (symbol_iterator != image.symbols().end())
      {
        if (unique_edges.emplace(function.entry, *call_site.target, true, symbol_iterator->name).second)
        {
          edges.push_back({function.entry, *call_site.target, true, symbol_iterator->name});
        }
      }
    }
  }

  std::sort(edges.begin(), edges.end(), [](const CallGraphEdge& left, const CallGraphEdge& right) {
    if (left.source != right.source)
    {
      return left.source < right.source;
    }
    if (left.target != right.target)
    {
      return left.target < right.target;
    }
    return left.target_name < right.target_name;
  });
  return edges;
}

}  // namespace

Result<ProgramAnalysis> ProgramAnalyzer::analyze(
    const BinaryImage& image,
    const DisassemblyResult& disassembly,
    const AnalysisOptions& options) const
{
  if (disassembly.instructions().empty())
  {
    return Result<ProgramAnalysis>::failure(Error::analysis("analysis requires disassembly input"));
  }

  const std::vector<SeedRecord> seeds =
      collectSeeds(image, disassembly, options.enable_prologue_heuristics);
  const std::map<Address, std::vector<FunctionSeed>> reasons = seedReasonMap(seeds);
  const std::map<Address, std::string> symbol_names = functionSymbolNames(image);

  std::set<Address> hard_boundary_seeds;
  for (const SeedRecord& seed : seeds)
  {
    if (seed.hard_boundary)
    {
      hard_boundary_seeds.insert(seed.seed.address);
    }
  }

  ProgramAnalysis analysis;
  std::set<Address> claimed_instructions;
  for (const SeedRecord& seed : seeds)
  {
    if (claimed_instructions.contains(seed.seed.address))
    {
      analysis.unused_seeds.push_back(seed.seed);
      continue;
    }

    RecoveredState recovered = recoverFunction(
        seed.seed.address,
        reasons.at(seed.seed.address),
        image,
        disassembly,
        hard_boundary_seeds,
        symbol_names);

    if (recovered.function.blocks.empty())
    {
      analysis.unused_seeds.push_back(seed.seed);
      continue;
    }

    bool overlaps = false;
    for (Address address : recovered.instructions)
    {
      if (claimed_instructions.contains(address))
      {
        overlaps = true;
        break;
      }
    }

    if (overlaps)
    {
      analysis.unused_seeds.push_back(seed.seed);
      continue;
    }

    claimed_instructions.insert(recovered.instructions.begin(), recovered.instructions.end());
    analysis.functions.push_back(std::move(recovered.function));
  }

  std::sort(
      analysis.functions.begin(),
      analysis.functions.end(),
      [](const RecoveredFunction& left, const RecoveredFunction& right) {
        return left.entry < right.entry;
      });

  analysis.call_graph = buildCallGraph(image, analysis.functions);
  for (RecoveredFunction& function : analysis.functions)
  {
    function.metrics = computeFunctionMetrics(function);
  }
  analysis.metrics = computeProgramMetrics(analysis.functions, analysis.call_graph);
  return Result<ProgramAnalysis>::success(std::move(analysis));
}

}  // namespace binaryatlas
