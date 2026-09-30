# About This Repository

## What it is

**AriaTutorial is a 20-chapter, bilingual tutorial for Aria, where every chapter ships a minimal demo you can compile, run, and see output from.**

It is not a tour of "what APIs exist". It answers three more practical questions:

1. **What problem does this framework solve**, and what does it explicitly cost (Chapter 1);
2. **How does each behaviour actually work** — printed by a runnable program, not promised by documentation;
3. **How do you use it in real code** — from a single `Property` to a complete application (Chapter 20).

## Who it is for

| Reader | Start here |
|---|---|
| Heard of Aria, deciding whether it fits | Chapter 1 (including the trade-off table), Chapter 20 (a full application) |
| Committed, needs to get productive | Chapter 2 to build, then Chapters 3-8 for the reactive core |
| Needs to attach a real UI | Chapters 9-14 (collections, forms, binding), 15-16 (adapters) |
| Cares about async and engineering rigour | Chapters 17-19 (coroutines, diagnostics, testing) |
| Just wants the code | Read `demos/` directly — one `main.cpp` per chapter |

Readers who know Vue, React, or Qt will move faster: `Property` is roughly `ref`, `Computed` is `computed`/`useMemo` (without the dependency array), and `ObservableList` resembles a collection with change events.

## How the content is organised

```
docs/          20 chapters, Chinese and English (NN-标题.md / NN-english-title.en.md)
demos/         one runnable target per chapter (ch01_bill … ch20_ecosystem)
demos/common/  the tutorial in-memory adapter shared across chapters
images/        one SVG figure per chapter, per language
```

**Articles and demos share one source, but generated in reverse**: the demo is written and genuinely run first, and the article is produced from its source and stdout. Every snippet in the text can be traced back to `demos/`.

**Figures follow the same constraint.** The diagram under each chapter title is not decoration; it is that chapter's conclusion drawn out -- a data flow, a dependency graph, a state machine, or a timeline. Every number in it (recompute counts, event counts, concurrency timing) comes from that chapter's real demo run, the same data as the ```text blocks in the text. Each figure is an SVG, so the wording can be changed; GitHub renders vector figures natively.

## Quality bar

Current CI builds and runs the default chapters through CTest with per-demo timeouts. Article snippets, output and SVG figures still require maintenance alongside the demos; the repository has no automatic gate comparing all article content character for character.

Historical output in the articles comes from Windows / MSVC 19.51. Timings, addresses and scheduling order can vary by environment; verify behaviour rather than expecting identical characters. On 2026-09-30, all 21 demo tests, including Qt6 / HTTP, passed on macOS / AppleClang 21.

## What is explicitly not covered

Drawn honestly, to avoid misleading anyone:

| Item | Note |
|---|---|
| **Chapter 15's Qt6 / HTTP demos** | Excluded by default; when enabled, CTest runs bounded `--smoke` checks, not full GUI or browser interactions |
| **AppKit / UIKit / JNI adapters** | These require building on their own platforms. The text gives wiring code and a comparison table, and **does not claim verification** |
| Visual and interaction testing | Belongs to UI testing, out of scope here |
| Every Aria API | This tutorial covers the mainline; numbered contract details live in Aria's `docs/` |

## How to build and verify everything

```bash
git clone https://github.com/dqsjqian/AriaTutorial.git
cd AriaTutorial

# Aria already installed
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# or point at a source tree
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DARIA_ROOT=../Aria

cmake --build build -j

# run any chapter and see the same output the article shows
./build/bin/ch01_bill
./build/bin/ch20_ecosystem
```

Convenience scripts: `scripts/run-all.sh` (macOS / Linux) or `scripts/run-all.ps1` (Windows).

## Relationship to Aria

This is the **teaching companion** to [Aria](https://github.com/dqsjqian/Aria), written by the framework's maintainer. It does not contain framework source.

- Framework, contract documents, API reference → [Aria](https://github.com/dqsjqian/Aria)
- Flagship cross-platform example → [AriaTools](https://github.com/dqsjqian/AriaTools)
- This repository → a learning path from zero to productive

## Contributing

Corrections, additional examples, and improvements to the bilingual text are welcome. Two rules:

1. **If you change a demo's source, update the corresponding article's code and output blocks in the same change**, and verify them during review;
2. **Never write "illustrative output" in an article** — either run it, or leave it out.

For a new chapter, an Issue outlining the plan first is appreciated.

## License

[MIT](LICENSE) © 2026 aria contributors
