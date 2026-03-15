# BinaryAtlas

Static binary analysis engine for recovering structure from native executables.

BinaryAtlas is a modern C++20 static analysis tool for reverse engineering compiled native binaries. It parses ELF64 x86_64 executables, disassembles executable regions with Capstone, recovers functions and control-flow structure heuristically, builds CFG and call graph views, computes structural metrics, and emits JSON and DOT for downstream tooling.

The project is aimed at engineers building runtime tooling, SDK integration layers, binary inspection pipelines, reverse-engineering infrastructure, performance tools, and low-level systems research workflows.

## Why BinaryAtlas Exists

Static binary analysis is useful long before decompilation. Many practical tasks only need a structural view:

- Which executable regions exist and how are they laid out?
- Which functions are likely present, even in stripped or partially stripped binaries?
- Where are the high-complexity or dispatcher-like regions?
- Which functions look like hook candidates for graphics or runtime integration?
- Are there vtable-like data regions or indirect dispatch sites worth triaging first?

BinaryAtlas focuses on those questions directly and keeps heuristic uncertainty explicit.

## Capabilities

- ELF64 parsing with x86_64 focus on Linux
- Safe extraction of headers, sections, segments, symbols, imports/exports abstraction, and relocations
- Section-based disassembly with Capstone
- Instruction metadata:
  - address
  - raw bytes
  - mnemonic and operand text
  - Capstone groups
  - flow classification
  - branch target when directly resolvable
- Basic block recovery
- Heuristic function recovery from:
  - entry point
  - symbol table entries
  - direct call targets
  - conservative prologue patterns
- Per-function CFG recovery
- Whole-program call graph construction
- Graph metrics:
  - function size
  - block and edge counts
  - cyclomatic complexity
  - SCC-based loop detection
  - in-degree and out-degree summaries
- Heuristic detections:
  - candidate virtual dispatch
  - candidate vtable regions
  - candidate jump tables
  - render and VR runtime hook candidates
  - engine fingerprints
  - high-complexity functions
  - dispatcher-like functions
- Output formats:
  - human-readable terminal output
  - stable JSON structures
  - DOT / Graphviz graph export
- Automated unit and integration tests with compiled fixture binaries

## Current Scope

BinaryAtlas is intentionally scoped for a credible first version.

- First-class support: ELF64 little-endian x86_64 on Linux
- Planned later: PE/COFF, Mach-O, AArch64, deeper indirect-control-flow resolution

## Limitations

BinaryAtlas is not a decompiler and does not claim exact source reconstruction.

- Function recovery is heuristic and can be incomplete
- Class and vtable recovery is suggestive, not authoritative
- Indirect calls and jumps may remain unresolved
- Stripped and aggressively optimized binaries reduce precision
- String-to-function attribution is conservative and incomplete
- Current release only supports ELF64 x86_64
- Hook detections are ranked suggestions, not proof of stable interception points

These limitations are reflected in the output schema through `partial`, `ambiguous`, unresolved edge types, and heuristic confidence scores.

## Developer Setup

BinaryAtlas is a CMake-based C++20 project and is configured to:

- prefer `clang++` when it is available
- fall back to `g++` when `clang++` is not installed
- prefer system-installed dependencies when they exist
- fall back to `FetchContent` for Capstone, nlohmann/json, and Catch2

That means a minimal development setup works even if `libcapstone-dev` is missing, but installing system packages is still recommended for faster and more predictable local builds.

### Required VS Code Extensions

- `ms-vscode.cmake-tools`
- `ms-vscode.cpptools`

Recommended extensions are already listed in [.vscode/extensions.json](.vscode/extensions.json).

### Linux Dependencies

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y \
  clang \
  cmake \
  g++ \
  gdb \
  graphviz \
  libcapstone-dev \
  make \
  ninja-build \
  pkg-config
```

Notes:

- `clang` provides `clang` and `clang++`
- `g++` is kept installed as the fallback compiler
- `gdb` is required for the Linux VS Code debug configuration
- `graphviz` is optional for building, but required to render `.dot` graph exports

### macOS Dependencies

Install Xcode Command Line Tools first:

```bash
xcode-select --install
```

Then install the remaining tools with Homebrew:

```bash
brew install llvm cmake capstone graphviz
```

Notes:

- Apple Clang from Xcode also works
- `lldb` is typically provided by Xcode Command Line Tools
- if Homebrew LLVM is not on your `PATH`, CMake will continue using Apple Clang unless you select the `clang-debug` preset explicitly

### Windows Dependencies

Recommended path:

- use VS Code with the Remote WSL extension
- develop inside WSL Ubuntu
- follow the Linux dependency instructions inside WSL

Alternative native path:

- install Visual Studio 2022 Build Tools or Visual Studio Community with C++ workload
- install CMake
- install Git
- optionally install Graphviz
- let BinaryAtlas fetch Capstone and test dependencies through CMake if you do not want to manage them separately

Windows note:

- the included VS Code launch configuration supports native MSVC debugging through `cppvsdbg`
- for the smoothest parity with the current ELF-focused toolchain, WSL is the preferred Windows workflow

## Build

### Standard CMake Build

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Exact `mkdir build && cd build && cmake .. && make` Flow

BinaryAtlas is configured to support the classic Makefile workflow directly.

```bash
mkdir -p build
cd build
cmake ..
make -j"$(nproc)"
ctest --output-on-failure
```

On systems without `nproc`, use a fixed job count:

```bash
make -j4
```

### Dependency Resolution Behavior

During configure, CMake will:

1. try to find a system Capstone install
2. try to find system `nlohmann_json` and `Catch2`
3. fall back to `FetchContent` when they are not found

This is handled in [CMakeLists.txt](CMakeLists.txt) and [Findcapstone.cmake](cmake/Findcapstone.cmake).

### Compiler Selection

Compiler selection is automatic:

- `clang` / `clang++` are preferred when available
- `gcc` / `g++` are used as fallback
- if `CC` or `CXX` are already set in your environment, CMake respects them
- if you are using a Visual Studio generator, CMake is left alone so MSVC can be selected normally

You can also force a compiler with presets:

```bash
cmake --preset clang-debug
cmake --build --preset clang-debug

cmake --preset gcc-debug
cmake --build --preset gcc-debug
```

## Quickstart

Inspect a binary:

```bash
./build/binaryatlas inspect ./build/tests/fixtures/bin/direct_calls
```

Example output:

```text
BinaryAtlas inspect
  Path: ./build/tests/fixtures/bin/direct_calls
  Format: elf / x86_64
  Entry: 0x401020
  Sections: 34
  Symbols: 38
  Relocations: 2
  Imports: 2
```

Recover functions:

```bash
./build/binaryatlas functions ./build/tests/fixtures/bin/direct_calls
```

Example output:

```text
Recovered functions: 11
Call graph edges: 3
  0x401106 callee blocks=1 complexity=1 calls=0 partial=no
  0x401118 caller blocks=2 complexity=1 calls=1 partial=no
  0x401136 main blocks=2 complexity=1 calls=1 partial=no
```

Run full analysis and emit JSON:

```bash
./build/binaryatlas analyze ./build/tests/fixtures/bin/direct_calls --json analysis.json
```

Export graphs:

```bash
./build/binaryatlas cfg ./build/tests/fixtures/bin/direct_calls --function caller --dot caller.dot
./build/binaryatlas callgraph ./build/tests/fixtures/bin/direct_calls --dot callgraph.dot
./build/binaryatlas analyze ./build/tests/fixtures/bin/direct_calls --full --dot-dir graphs
```

Run heuristics:

```bash
./build/binaryatlas heuristics ./build/tests/fixtures/bin/graphics_keywords
```

Example output:

```text
Heuristic findings: 5
  [candidate_engine_fingerprint] confidence=0.55
  [candidate_render_api_hook] confidence=0.68 function=0x401106
  [candidate_vr_runtime_hook] confidence=0.5
```

## CLI

Subcommands:

- `inspect`
- `disasm`
- `functions`
- `cfg`
- `callgraph`
- `heuristics`
- `analyze`

Examples:

```bash
binaryatlas inspect ./a.out
binaryatlas disasm ./a.out --section .text
binaryatlas functions ./a.out --json
binaryatlas cfg ./a.out --function 0x401000 --dot out.dot
binaryatlas callgraph ./a.out --dot callgraph.dot
binaryatlas heuristics ./a.out --json
binaryatlas analyze ./a.out --full --json out.json --dot-dir graphs
```

## Running The CLI

After building, the CLI entry point is:

```bash
./build/binaryatlas
```

Common commands:

```bash
./build/binaryatlas inspect ./build/tests/fixtures/bin/direct_calls
./build/binaryatlas functions ./build/tests/fixtures/bin/direct_calls
./build/binaryatlas heuristics ./build/tests/fixtures/bin/graphics_keywords
./build/binaryatlas analyze ./build/tests/fixtures/bin/direct_calls --full --dot-dir graphs
```

## JSON Example

BinaryAtlas emits numeric addresses together with a hex string for deterministic machine and human consumption.

```json
{
  "metadata": {
    "format": "elf",
    "architecture": "x86_64",
    "entry_point": {
      "value": 4198432,
      "hex": "0x401020"
    }
  },
  "analysis": {
    "functions": [
      {
        "name": "caller",
        "entry": {
          "value": 4198680,
          "hex": "0x401118"
        },
        "metrics": {
          "block_count": 2,
          "cyclomatic_complexity": 1
        }
      }
    ],
    "call_graph": [
      {
        "source": {
          "value": 4198680,
          "hex": "0x401118"
        },
        "target": {
          "value": 4198662,
          "hex": "0x401106"
        },
        "target_name": "callee"
      }
    ]
  }
}
```

## DOT / Graph Export

CFG and call graph exports are plain DOT and are easy to render with Graphviz:

```bash
dot -Tpng caller.dot -o caller.png
dot -Tsvg callgraph.dot -o callgraph.svg
```

## VS Code

BinaryAtlas includes a ready-to-use [.vscode](.vscode) folder and [CMakePresets.json](CMakePresets.json).

Included files:

- [.vscode/settings.json](.vscode/settings.json)
- [.vscode/tasks.json](.vscode/tasks.json)
- [.vscode/launch.json](.vscode/launch.json)
- [.vscode/extensions.json](.vscode/extensions.json)

### Open And Configure

1. Open the repository root in VS Code.
2. Install the recommended extensions when prompted.
3. Let CMake Tools auto-configure the project, or run `CMake: Configure`.
4. Choose the `default` preset unless you explicitly want `clang-debug`, `gcc-debug`, or `release`.

`CMake: Build` will build the project using the active preset and place outputs under `build/vscode/<preset-name>/`.

### IntelliSense

IntelliSense is configured through:

- CMake Tools as the configuration provider
- `compile_commands.json` copied to the repository root

That means include paths, compile definitions, and warning flags come from the actual CMake configuration instead of being duplicated manually in VS Code settings.

### VS Code Tasks

The repository provides these tasks:

- `CMake Configure`
- `CMake Build`
- `CMake Test`
- `Build With make`

You can run them from `Terminal: Run Task`.

### Debugging In VS Code

The debug configurations in [.vscode/launch.json](.vscode/launch.json) are set up for:

- Linux and WSL with `gdb`
- macOS with `lldb`
- Windows with native MSVC debugging

Debug flow:

1. Build the project with `CMake: Build`
2. In the CMake Tools status bar, set the launch target to `binaryatlas`
3. Press `F5`
4. Choose `Debug BinaryAtlas CLI` on Linux/macOS/WSL or `Debug BinaryAtlas CLI (MSVC)` on Windows
5. Add the CLI arguments you want in the debug configuration before launching

Example debug arguments:

```bash
inspect ./build/tests/fixtures/bin/direct_calls --verbose
```

You can edit the arguments in [.vscode/launch.json](.vscode/launch.json) for your own fixture or binary paths.

### CMake Tools Compatibility

This repository is configured to work directly with:

- `CMake: Configure`
- `CMake: Build`
- `CMake: Run Without Debugging`
- `CMake: Debug`

through the included [CMakePresets.json](CMakePresets.json) and `.vscode` settings.

## Architecture Summary

Pipeline:

1. Parse binary into a `BinaryImage`
2. Disassemble executable sections into an indexed instruction stream
3. Recover basic blocks and functions heuristically
4. Build CFG and call graph structures
5. Compute graph metrics
6. Run heuristic detectors over structural, symbolic, and string evidence
7. Render terminal, JSON, and DOT outputs

More detail:

- [docs/architecture.md](docs/architecture.md)
- [docs/heuristics.md](docs/heuristics.md)
- [docs/examples.md](docs/examples.md)
- [docs/roadmap.md](docs/roadmap.md)

## Repository Layout

```text
include/binaryatlas/
  core/        shared binary model and orchestration
  format/      file format backends
  disasm/      Capstone-backed disassembly
  analysis/    CFG, function recovery, graph metrics
  graph/       DOT export
  heuristics/  scored heuristic detectors
  output/      JSON and terminal renderers
  util/        low-level helpers
src/           implementations
tests/         unit and integration tests with compiled fixtures
docs/          design notes and user documentation
examples/      usage examples
```

## Screenshots

Screenshot placeholders intentionally included for future project polish:

- `[placeholder] CLI summary on a stripped game binary`
- `[placeholder] CFG rendered from DOT/Graphviz`
- `[placeholder] Heuristic finding report for a graphics-heavy sample`

## Disclaimer

BinaryAtlas is a static analysis and reverse-engineering research tool. It is not a decompiler, not a vulnerability scanner, and not a source-code recovery system. Treat recovered structure and all heuristic findings as analyst assistance, not ground truth.

## Roadmap

Highlights:

- PE/COFF backend
- Mach-O backend
- AArch64 support
- better indirect branch and jump-table resolution
- string xrefs and richer data-flow recovery
- improved class and RTTI recovery
- plugin interfaces for custom detectors and exporters

See [docs/roadmap.md](docs/roadmap.md).
