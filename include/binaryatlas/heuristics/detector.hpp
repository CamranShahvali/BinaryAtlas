#pragma once

#include <vector>

#include "binaryatlas/analysis/analysis_result.hpp"
#include "binaryatlas/core/binary_image.hpp"
#include "binaryatlas/disasm/instruction.hpp"
#include "binaryatlas/heuristics/finding.hpp"

namespace binaryatlas
{

struct HeuristicOptions
{
  std::vector<std::string> graphics_keywords{"directx", "dxgi",        "d3d",    "vulkan",
                                             "opengl",  "swapbuffers", "present"};
  std::vector<std::string> vr_keywords{"openxr", "openvr", "steamvr", "xr"};
  std::vector<std::string> engine_keywords{"unity", "unreal", "ue4", "ue5", "cryengine", "godot"};
  std::size_t high_complexity_threshold{8};
  std::size_t dispatcher_out_degree_threshold{5};
};

class HeuristicDetector
{
public:
  [[nodiscard]] std::vector<HeuristicFinding> analyze(const BinaryImage& image,
                                                      const DisassemblyResult& disassembly,
                                                      const ProgramAnalysis& analysis,
                                                      const HeuristicOptions& options = {}) const;
};

} // namespace binaryatlas
