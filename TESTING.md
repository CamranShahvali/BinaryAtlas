# BinaryAtlas Testing Guide

This document is the practical verification playbook for BinaryAtlas.

It covers:

- local dependency setup
- build commands
- CLI verification commands
- unit and integration test execution
- fixture binaries
- expected outputs
- troubleshooting

## Build Requirements

BinaryAtlas is a CMake-based C++20 project.

Core requirements:

- CMake 3.24+
- C++20-capable compiler
- Capstone
- nlohmann/json
- Catch2 for tests

The repository prefers system packages when available and falls back to `FetchContent` for Capstone, nlohmann/json, and Catch2.

### Linux

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

### macOS

Install Xcode Command Line Tools:

```bash
xcode-select --install
```

Install packages with Homebrew:

```bash
brew install llvm cmake capstone graphviz
```

### Windows

Recommended:

- use WSL2 with Ubuntu
- install the Linux dependencies inside WSL
- use VS Code Remote WSL

Native alternative:

- Visual Studio 2022 Build Tools or Visual Studio Community with C++ workload
- CMake
- Graphviz optional for rendering `.dot` files

## Build Instructions

### Linux and macOS

Exact classic workflow:

```bash
mkdir build
cd build
cmake ..
make
```

Parallel build:

```bash
mkdir -p build
cd build
cmake ..
make -j4
```

Run tests:

```bash
ctest --output-on-failure
```

Alternative from the repository root:

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Windows Native

Use CMake build mode instead of `make`:

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

### Windows WSL

Use the Linux workflow inside WSL:

```bash
mkdir build
cd build
cmake ..
make
ctest --output-on-failure
```

## Compiler Selection

Compiler selection is automatic:

- `clang++` is preferred when available
- `g++` is used as fallback
- if `CC` and `CXX` are set, CMake respects them
- Visual Studio generators are left alone so MSVC can be selected normally

Examples:

```bash
CC=clang CXX=clang++ cmake -S . -B build
cmake --build build
```

```bash
CC=gcc CXX=g++ cmake -S . -B build
cmake --build build
```

## VS Code Workflow

The repository already includes:

- `.vscode/settings.json`
- `.vscode/tasks.json`
- `.vscode/launch.json`
- `CMakePresets.json`

Supported VS Code flows:

- `CMake: Configure`
- `CMake: Build`
- `CMake: Run Tests`
- launch the CLI through the configured debugger

Recommended extensions:

- `ms-vscode.cmake-tools`
- `ms-vscode.cpptools`

Debuggers:

- Linux: `gdb`
- macOS: `lldb`
- Windows MSVC: `cppvsdbg`

## CLI Commands

BinaryAtlas currently exposes these commands:

```bash
binaryatlas inspect <binary>
binaryatlas disasm <binary>
binaryatlas functions <binary>
binaryatlas cfg <binary> --function <addr|name>
binaryatlas callgraph <binary>
binaryatlas heuristics <binary>
binaryatlas analyze <binary>
```

Common options:

- `--json [path]`
- `--verbose`
- `--section <name>` for `disasm`
- `--start <addr>` and `--end <addr>` for `disasm`
- `--dot <path>` for `cfg` and `callgraph`
- `--dot-dir <dir>` for `analyze`
- `--full` for `analyze`

## Testing Checklist

Verify each of these areas before calling the repository healthy:

- ELF parsing
- section and segment extraction
- symbol parsing
- address translation
- disassembly over executable sections
- instruction flow classification
- function recovery
- basic block splitting
- CFG generation
- call graph generation
- graph metrics
- SCC and loop detection
- heuristic analysis
- JSON serialization
- DOT graph export
- CLI argument handling
- malformed input handling
- stripped binary handling

## Unit and Integration Tests

The repository already contains a real automated test suite.

Run from the build directory:

```bash
ctest --output-on-failure
```

Run from the repository root:

```bash
ctest --test-dir build --output-on-failure
```

Makefile shortcut:

```bash
cd build
make test
```

What the current suite covers:

- ELF header and section parsing
- symbol parsing
- malformed and truncated ELF handling
- address translation
- disassembler classification
- invalid instruction handling
- non-executable disassembly rejection
- SCC detection
- cyclomatic complexity
- JSON schema emission
- DOT export generation
- heuristic scoring
- integration analysis on fixture binaries

## Fixture Binaries

Fixture binaries are built automatically with the test target under:

```text
build/tests/fixtures/bin/
```

Current fixtures:

- `straight_line`
- `straight_line_cpp`
- `if_else`
- `branching_cpp`
- `loop`
- `loop_cpp`
- `switch_case`
- `direct_calls`
- `indirect_call`
- `virtual_dispatch`
- `graphics_keywords`
- `direct_calls_stripped`

They cover the requested cases:

- simple function
- simple C++ function
- loop
- C++ loop
- branching
- C++ branching
- direct calls
- indirect function-pointer call
- virtual method dispatch
- graphics and runtime keyword heuristics
- stripped binary recovery

## Real Verification Commands

The following commands are safe local smoke tests.

### Inspect ELF Metadata

```bash
./build/binaryatlas inspect /bin/ls
```

Expected result:

- reports `Format: elf / x86_64`
- shows a valid entry point
- reports non-zero sections

### Disassemble Executable Code

```bash
./build/binaryatlas disasm /bin/ls --section .text
```

Expected result:

- reports a non-zero instruction count
- shows instruction addresses and mnemonics
- does not abort on invalid bytes

### Recover Functions

```bash
./build/binaryatlas functions /bin/ls
```

Expected result:

- prints recovered function count
- prints call graph edge count
- includes `_start` and multiple recovered subroutines

### Export a Single CFG

```bash
./build/binaryatlas cfg /bin/ls --function _start --dot /tmp/ls_start.dot
```

Expected result:

- terminal output describes the CFG for `_start`
- `/tmp/ls_start.dot` exists and is non-empty

### Export the Global Call Graph

```bash
./build/binaryatlas callgraph /bin/ls --dot /tmp/ls_callgraph.dot
```

Expected result:

- terminal output lists caller -> callee edges
- `/tmp/ls_callgraph.dot` exists and is non-empty

### Run Heuristics

```bash
./build/binaryatlas heuristics /bin/ls
```

Expected result:

- reports a heuristic finding count
- may include jump-table, dispatcher-like, or virtual-dispatch candidates

### Full Analysis with JSON and DOT

```bash
./build/binaryatlas analyze /bin/ls --full --json /tmp/ls_analysis.json --dot-dir /tmp/ls_graphs
```

Expected result:

- `/tmp/ls_analysis.json` exists and is non-empty
- `/tmp/ls_graphs/callgraph.dot` exists
- per-function DOT files are produced in `/tmp/ls_graphs`

### Fixture-Based Verification

Recover direct-call structure:

```bash
./build/binaryatlas functions ./build/tests/fixtures/bin/direct_calls
```

Check loop recovery:

```bash
./build/binaryatlas functions ./build/tests/fixtures/bin/loop
```

Check C++ branching recovery:

```bash
./build/binaryatlas functions ./build/tests/fixtures/bin/branching_cpp
```

Check virtual dispatch heuristics:

```bash
./build/binaryatlas heuristics ./build/tests/fixtures/bin/virtual_dispatch
```

Check graphics/runtime keyword heuristics:

```bash
./build/binaryatlas heuristics ./build/tests/fixtures/bin/graphics_keywords
```

Check stripped binary recovery:

```bash
./build/binaryatlas functions ./build/tests/fixtures/bin/direct_calls_stripped
```

### JSON Output Checks

Inspect JSON:

```bash
./build/binaryatlas inspect /bin/ls --json /tmp/inspect.json
./build/binaryatlas functions /bin/ls --json /tmp/functions.json
./build/binaryatlas heuristics /bin/ls --json /tmp/heuristics.json
```

Expected result:

- each file is valid JSON
- inspect JSON contains metadata, sections, symbols, and relocations
- function JSON contains recovered functions, blocks, and call graph edges
- heuristics JSON contains categories, confidence, evidence, and explanations

## Expected Test Outcomes

Healthy repository signals:

- `cmake ..` configures without missing include path errors
- `make` completes without link failures
- `ctest` passes all discovered tests
- fixture binaries are produced under `build/tests/fixtures/bin`
- `inspect` works on a system ELF such as `/bin/ls`
- `analyze --full` produces JSON and DOT output
- VS Code `CMake: Configure` and `CMake: Build` succeed

## Troubleshooting

### CMake Cannot Find a Compiler

Install `clang` and `g++`, then retry:

```bash
sudo apt-get install -y clang g++
```

Or force one explicitly:

```bash
CC=clang CXX=clang++ cmake -S . -B build
```

### Capstone Is Missing

Install the system package if you want explicit system resolution:

```bash
sudo apt-get install -y libcapstone-dev
```

If that package is missing, BinaryAtlas should still build by fetching Capstone automatically through CMake.

### Graphviz Cannot Render DOT Files

Install Graphviz:

```bash
sudo apt-get install -y graphviz
```

Render an exported graph:

```bash
dot -Tpng /tmp/ls_callgraph.dot -o /tmp/ls_callgraph.png
```

### Generator or Cache Mismatch

If you switch compilers or generators and CMake behaves inconsistently, remove the build directory and configure again:

```bash
rm -rf build
mkdir build
cd build
cmake ..
```

### VS Code Debugging Does Not Start

Check that the debugger is installed:

- Linux: `gdb`
- macOS: `lldb`
- Windows native: MSVC debugger

Then rebuild from VS Code and launch `Debug BinaryAtlas CLI`.

## Audit Summary

Repository audit status at the time of writing:

- CMake configuration verified
- C++20 configuration verified
- dependency resolution verified
- CLI entrypoints verified
- CTest suite verified
- fixture binaries verified
- VS Code CMake Tools workflow configured
- VS Code debugger configuration present
