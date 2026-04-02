# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Wuffs (Wrangling Untrusted File Formats Safely) is a memory-safe programming language and standard library for decoding/encoding untrusted file formats. Wuffs source (`.wuffs` files) transpiles to C99 code. The generated C is distributed as a single-file library (`release/c/wuffs-*.c`). Safety guarantees (buffer overflows, integer overflow, null dereferences) are enforced at compile time with zero runtime overhead.

## Build Commands

```bash
# Install Go-based toolchain (wuffs, wuffsfmt)
go install ./cmd/wuffs*

# Regenerate C code after editing .wuffs files
wuffs gen std/...            # all modules
wuffs gen std/gif            # single module

# Run tests
wuffs test                   # all tests
wuffs test std/gif           # single module
wuffs test -mimic            # compare against reference C libraries (giflib, libpng, etc.)

# Run benchmarks
wuffs bench std/gif          # single module
wuffs bench -mimic           # compare performance vs reference libs

# Run Go unit tests (for toolchain code in lang/)
go test ./...

# Build example programs
./build-example.sh example/zcat    # single example
./build-example.sh                 # all examples

# Build fuzz harnesses
./build-fuzz.sh

# Full CI check (run before submitting a PR)
./build-all.sh
```

## Architecture

**Toolchain (`lang/`)** — Go code implementing the Wuffs-to-C compiler:
- `lang/parse` — parser producing AST
- `lang/check` — type checker, bounds checker, proof/assertion verifier
- `lang/generate` — C code generation orchestration
- `lang/ast` — AST node definitions
- `lang/builtin` — built-in type and function signatures
- `lang/token` — tokenizer
- `lang/wuffsroot` — repository root discovery

**Standard Library (`std/`)** — Wuffs source for codecs: image formats (gif, png, jpeg, bmp, webp, qoi, targa, wbmp, vp8, etc2, thumbhash), compression (deflate, gzip, zlib, bzip2, lzma, lzip, lzw, xz), checksums/hashes (crc32, crc64, adler32, sha256, xxhash32/64), data formats (json, cbor, netpbm, nie).

**Generated Output (`release/c/`)** — Pre-generated single-file C libraries checked into the repo. Users `#include` these directly; define `WUFFS_IMPLEMENTATION` to compile the implementation, not just headers.

**Internal C Templates (`internal/cgen/`)** — Base C code and auxiliary C++ helpers that get incorporated into generated output.

**Tests (`test/c/`)** — C test files per codec in `test/c/std/`. Mimic tests in `test/c/mimiclib/` compare against third-party C libraries. Test data in `test/data/`.

**CLI Tools (`cmd/`)** — `wuffs` (gen/test/bench/genlib), `wuffs-c`, `wuffsfmt` (auto-formatter), `ractool`, `dumbindent`.

**Supporting Go Libraries (`lib/`)** — Go wrappers and utilities used by tools and examples.

## Key Language Concepts

- **Hermetic**: No I/O, no memory allocation, no syscalls. Callers provide all buffers.
- **Coroutines**: Methods marked `?` can suspend on `$short read`/`$short write`; callers refill buffers and resume.
- **Refinement types**: e.g. `base.u32[..= 255]` constrains value ranges, verified at compile time via interval arithmetic.
- **Facts and assertions**: Compile-time proof system; `assert` statements with named axioms for bounds safety.
- **Effects**: `!` marks impure methods, `?` marks coroutines.
- **Syntax differences from C**: `and`/`or`/`not` for logical ops, `<>` for not-equals, `~mod+`/`~sat+` for modular/saturating arithmetic, no operator precedence (explicit parens required).

## Code Style

- Wuffs source: auto-formatted with `wuffsfmt`
- C/C++ code: Chromium style (`.clang-format` in repo root)
- License: Apache-2.0 OR MIT (dual-licensed)
