# Examples

## Inspect Binary Structure

```bash
binaryatlas inspect ./target.bin
binaryatlas inspect ./target.bin --json inspect.json
binaryatlas inspect ./target.bin --verbose
```

Use when you want:

- metadata
- section and segment inventory
- symbol overview
- import/export summary

## Disassemble One Section

```bash
binaryatlas disasm ./target.bin --section .text
binaryatlas disasm ./target.bin --section .text --start 0x401100 --end 0x4011f0
binaryatlas disasm ./target.bin --json disasm.json
```

Useful for:

- focused code review
- checking control-transfer classification
- validating section boundaries and decode quality

## Recover Functions

```bash
binaryatlas functions ./target.bin
binaryatlas functions ./target.bin --verbose
binaryatlas functions ./target.bin --json functions.json
```

Interpretation tips:

- `partial=yes` means recovery encountered unresolved control flow or invalid bytes
- block counts and complexity help prioritize review order
- call counts reflect direct call targets that were recovered

## Export A Function CFG

```bash
binaryatlas cfg ./target.bin --function 0x401118 --dot caller.dot
binaryatlas cfg ./target.bin --function caller
```

You can select a function by:

- hex entry address
- recovered function name

Render with Graphviz:

```bash
dot -Tsvg caller.dot -o caller.svg
```

## Export The Call Graph

```bash
binaryatlas callgraph ./target.bin --dot callgraph.dot
dot -Tpng callgraph.dot -o callgraph.png
```

Call graph export is useful for:

- identifying leaf helpers
- spotting dispatcher or setup hubs
- triaging game-engine or runtime integration flows

## Run Heuristics

```bash
binaryatlas heuristics ./target.bin
binaryatlas heuristics ./target.bin --json findings.json
```

Reading findings:

- treat confidence as ranking only
- read the evidence list before trusting the category
- expect some false positives on compiler-generated runtime helpers

## Full Analysis Pass

```bash
binaryatlas analyze ./target.bin --full --dot-dir graphs --json analysis.json
```

This is the easiest way to generate:

- structural JSON
- function CFG DOT files
- global call graph DOT
- heuristic findings in one run
