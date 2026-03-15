# Roadmap

BinaryAtlas is intentionally shipping a strong first version around ELF64 x86_64 rather than pretending to solve every backend and every recovery problem up front.

## Near Term

- PE/COFF parser backend
- Mach-O parser backend
- AArch64 disassembly support
- richer import/export abstraction per format
- better PLT/GOT understanding for ELF call edges
- improved stripped-binary function seeding

## Analysis Depth

- indirect jump table resolution
- switch reconstruction with data-backed table decoding
- string xrefs from code to data
- more precise data-reference recovery for PIC code
- dominator trees and optional dominance frontiers
- interprocedural reachability summaries

## C++ Recovery

- improved virtual call recovery
- RTTI-aware analysis where available
- better vtable grouping
- constructor and destructor pattern recognition
- class hierarchy hints from vtable and call usage

## Extensibility

- plugin API for custom detectors
- configurable output schemas
- custom keyword packs for engines and runtime APIs
- scripting bridge for batch analysis

## Visualization

- graph filtering and clustering
- richer DOT labels
- optional web or desktop visualization frontend

## Testing and Hardening

- malformed-file corpus expansion
- fuzz harnesses around parser and disassembler adapters
- larger integration corpus across optimization levels
- regression corpus for stripped and LTO-heavy binaries

## Non-Goals

Still out of scope for the foreseeable future:

- full decompilation
- exact source reconstruction
- exact class recovery guarantees
- exact indirect-control-flow resolution for arbitrary optimized binaries
