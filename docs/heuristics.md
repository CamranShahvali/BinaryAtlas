# Heuristics

## Philosophy

BinaryAtlas heuristics are intentionally labeled as heuristics. They are not proofs.

Each finding includes:

- category
- confidence score
- evidence strings
- explanation text

The confidence is a ranking aid, not a probability guarantee.

## Implemented Categories

### `candidate_virtual_dispatch`

Signals:

- indirect calls inside recovered functions
- memory-load patterns that resemble object-to-vtable-to-slot dispatch
- optional boost when the binary also contains vtable-like data regions

Typical evidence:

- `indirect_call=rcx`
- `indirect_call=qword ptr [rax + 0x10]`

False positives:

- callback trampolines
- generic function-pointer dispatch
- runtime startup helpers

False negatives:

- heavily optimized inlined dispatch
- indirect calls hidden behind register shuffles not recognized by the simple shape check

### `candidate_vtable_region`

Signals:

- non-executable allocated sections containing consecutive pointer-sized values that each point into executable code

Typical evidence:

- number of consecutive executable pointers
- section name
- region address

False positives:

- GOT-like data
- callback tables
- jump tables stored in writable or read-only data

False negatives:

- small vtables
- vtables split by metadata or relocations
- stripped layouts where pointers are indirect or encoded

### `candidate_jump_table`

Signals:

- unresolved indirect jump in a multi-block function

BinaryAtlas does not currently resolve switch tables exactly. The finding is a triage signal that a switch or dispatcher shape may be present.

False positives:

- dispatch trampolines
- runtime-generated tables
- compiler helper thunks

False negatives:

- switches lowered as branch chains
- jump tables resolved through relocations or data flow not modeled yet

### `candidate_render_api_hook`

Signals:

- graphics-related keywords in function names
- graphics-related keywords in referenced or binary-level strings
- terms such as `present`, `swap`, `render`, `directx`, `dxgi`, `vulkan`, `opengl`

Purpose:

- prioritize likely render-path integration points for manual RE and instrumentation work

False positives:

- diagnostic strings
- unrelated symbols sharing graphics-like words

False negatives:

- stripped binaries with no useful strings
- engines that avoid human-readable API strings

### `candidate_vr_runtime_hook`

Signals:

- VR runtime keywords in strings or symbols
- terms such as `openxr`, `openvr`, `steamvr`, `xr`

Purpose:

- flag binaries with likely XR integration points

False positives:

- documentation or marketing strings compiled into a binary

False negatives:

- custom runtime wrappers with no obvious naming

### `candidate_engine_fingerprint`

Signals:

- engine keywords in strings or symbols
- terms such as `unity`, `unreal`, `ue4`, `ue5`, `godot`

Purpose:

- binary-level triage, not exact engine identification

False positives:

- plugin names
- test strings
- unrelated asset names

False negatives:

- stripped or obfuscated production builds
- engines wrapped behind proprietary naming

### `high_complexity_function`

Signals:

- cyclomatic complexity at or above the configured threshold

Purpose:

- prioritize structurally dense functions for manual review

This is not a semantic heuristic. It is a structural triage heuristic.

### `dispatcher_like_function`

Signals:

- high CFG fanout
- high call out-degree

Purpose:

- identify routing, command dispatch, event distribution, and large-switch style functions

False positives:

- error handling hubs
- compiler-generated glue with many exits

## Keyword Configuration

The detector currently exposes keyword lists via `HeuristicOptions`:

- graphics keywords
- VR keywords
- engine keywords

This keeps tuning local and testable without hard-coding policy into the CLI.

## Why The Heuristics Are Approximate

Static binary analysis sees post-compilation artifacts:

- symbols may be missing
- types are erased
- indirect data flow is incomplete
- optimizer transforms can obscure source-level structure
- relocations and loader behavior are only partially modeled

Because of that, BinaryAtlas surfaces uncertainty explicitly rather than claiming certainty it cannot justify.
