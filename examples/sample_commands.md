# Sample Commands

```bash
# Inspect binary structure
binaryatlas inspect ./a.out

# Disassemble only the main text section
binaryatlas disasm ./a.out --section .text

# Emit full disassembly as JSON
binaryatlas disasm ./a.out --json disasm.json

# Recover functions and print a summary
binaryatlas functions ./a.out

# Recover functions and store them as JSON
binaryatlas functions ./a.out --json functions.json

# Export one function CFG by address
binaryatlas cfg ./a.out --function 0x401000 --dot cfg.dot

# Export one function CFG by recovered name
binaryatlas cfg ./a.out --function main --dot main.dot

# Export the global call graph
binaryatlas callgraph ./a.out --dot callgraph.dot

# Run heuristic detectors
binaryatlas heuristics ./a.out

# Full analysis with JSON and DOT outputs
binaryatlas analyze ./a.out --full --json analysis.json --dot-dir graphs
```
