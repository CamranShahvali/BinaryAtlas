# Architecture

## Overview

BinaryAtlas is organized as a staged static-analysis pipeline:

1. `format/`
   Parses an input file into a structured `BinaryImage`
2. `disasm/`
   Decodes executable bytes into a searchable instruction stream
3. `analysis/`
   Recovers blocks, functions, CFGs, call graphs, and metrics
4. `heuristics/`
   Scores higher-level reverse-engineering signals from structural evidence
5. `output/`
   Renders terminal, JSON, and DOT views
6. `cli/`
   Exposes the pipeline through focused subcommands

The code is intentionally separated so future PE/COFF and Mach-O backends can reuse the same downstream analysis layers.

## Core Data Model

`BinaryImage` is the central parsed-binary model.

It owns:

- metadata
- sections
- segments
- symbols
- relocations
- import/export name sets
- extracted strings
- raw file bytes for address translation and section-backed reads

Key helpers:

- virtual address to file offset translation
- file offset to virtual address translation
- executable-range enumeration
- section lookup by name or containing address

This model is format-neutral enough for later backend additions.

## File Format Layer

Current backend:

- `ElfParser`

Current support:

- ELF64
- little-endian
- x86_64
- Linux-style executable layout

Parser responsibilities:

- validate ELF header and target architecture
- parse section headers and names
- parse program headers
- parse symbol tables
- parse relocations when present
- expose import/export abstractions from ELF symbol state
- extract printable strings from allocated sections

Tradeoff:

- Extended ELF section numbering is currently rejected rather than partially supported. This keeps the parser small and explicit for the first release.

## Disassembly Layer

BinaryAtlas uses Capstone as a decoding backend.

Important choices:

- byte-by-byte resilient decoding
- invalid bytes are recorded as synthetic invalid instructions instead of aborting the section
- instructions preserve bytes, mnemonic, operand text, group names, and flow classification

Flow classification currently distinguishes:

- normal
- direct call
- indirect call
- conditional jump
- unconditional jump
- indirect jump
- return
- trap
- invalid

The disassembly result is indexed by instruction address for later CFG recovery.

## Function and CFG Recovery

Recovery is intentionally hybrid rather than purely symbol-driven or purely recursive-descent.

Function seeds:

- entry point
- defined function symbols
- direct call targets inside executable ranges
- conservative frame-prologue matches

Block starts:

- function entry
- direct branch targets
- fallthrough after conditional branches
- fallthrough after calls

Block termination:

- calls
- direct and indirect jumps
- returns
- traps
- invalid bytes

Important behavior:

- unresolved indirect branches are preserved explicitly
- tail-jump-like transfers into hard symbol boundaries are treated conservatively
- overlapping seeds are skipped and surfaced as unused seeds

This design favors deterministic, explainable recovery over aggressive speculation.

## Graph and Metrics Layer

Graph algorithms are isolated in `analysis/graph_algorithms.*`.

Implemented today:

- depth-first reachability
- Tarjan SCC
- connected-component counting

Metrics:

- block count
- edge count
- instruction count
- approximate size in bytes
- maximum in-degree and out-degree
- average out-degree
- call out-degree
- cyclomatic complexity
- loop count from SCCs

Cyclomatic complexity uses the standard `E - N + 2P` formulation with an explicit synthetic exit treatment for non-internal exits.

## Heuristics Layer

The heuristics pass is deliberately separate from structural recovery.

Reasons:

- keeps the recovered graph model format- and policy-neutral
- makes uncertainty explicit
- allows later replacement or tuning without destabilizing core parsing

Current evidence sources:

- symbol names
- import names
- extracted strings
- function metrics
- unresolved indirect control flow
- vtable-like pointer runs in data sections
- indirect call patterns in recovered functions

Each finding carries:

- category
- optional address
- optional function
- confidence
- evidence strings
- explanation

## Output Layer

`output/`

- `JsonWriter`
- `TerminalRenderer`

`graph/`

- `DotExporter`

The JSON format is stable enough for test assertions and machine consumers. DOT is emitted as plain text to keep Graphviz optional.

## CLI Layer

CLI subcommands are intentionally narrow:

- `inspect`
- `disasm`
- `functions`
- `cfg`
- `callgraph`
- `heuristics`
- `analyze`

The CLI is implemented without an external parsing dependency to keep the build simple and dependency-light.

## Error Handling

The codebase uses explicit `Result<T>` and `Status` wrappers instead of exceptions for ordinary operational failures.

Typical failure classes:

- malformed file input
- unsupported binary format
- invalid section selection
- failed disassembly initialization
- output file write failure

This keeps the CLI behavior deterministic and easy to test.

## Design Tradeoffs

Chosen:

- precise, bounded heuristics over aggressive whole-program speculation
- deterministic output ordering for tests
- static library plus thin CLI executable
- byte-backed address translation in the parsed image model

Deferred:

- PE/COFF and Mach-O loaders
- AArch64 decoding
- richer relocation and PLT/GOT interpretation
- indirect control-flow resolution
- interprocedural data-flow
- dominator trees and SSA-style analysis

## Future Backend Expansion

PE/COFF and Mach-O should reuse:

- `BinaryImage`
- disassembly layer
- CFG and function recovery
- metrics and heuristics
- JSON and DOT renderers

Format-specific work will mainly live in:

- header parsing
- symbol/import/export modeling
- address translation semantics
- relocation and loader metadata handling
